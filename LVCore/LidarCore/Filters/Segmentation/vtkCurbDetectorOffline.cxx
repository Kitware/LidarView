/*=========================================================================

  Program:   LidarView
  Module:    vtkCurbDetectorOffline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCurbDetectorOffline.h"

#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <Eigen/Dense>

vtkStandardNewMacro(vtkCurbDetectorOffline);

namespace
{
//-----------------------------------------------------------------------------
// Local coordinate frame attached to one trajectory segment slice.
// Axes are expressed in world coordinates.
struct SliceFrame
{
  // Midpoint of the trajectory segment (meters).
  Eigen::Vector3d Center = Eigen::Vector3d::Zero();
  // Slice normal aligned with the local trajectory direction.
  Eigen::Vector3d Normal = Eigen::Vector3d::Zero();
  // In-plane lateral axis, used for signed left/right projection.
  Eigen::Vector3d Lateral = Eigen::Vector3d::Zero();
  // In-plane vertical axis, used for signed height projection.
  Eigen::Vector3d Vertical = Eigen::Vector3d::Zero();
};

//-----------------------------------------------------------------------------
// Cached projection of one input point into a slice local frame.
// Dl and Dvp are signed in-plane coordinates in meters.
struct ProjPoint
{
  // Source point id in the aggregated cloud.
  vtkIdType Pid = -1;
  // Signed lateral coordinate on the slice (meters).
  double Dl = 0.0;
  // Signed vertical coordinate on the slice (meters).
  double Dvp = 0.0;
};

//-----------------------------------------------------------------------------
// Per-slice 1D lateral profile for smoothing and curb detection.
// RepresentativeIndices stores per-bin sample indices in SortedPoints.
struct SliceProfile
{
  // Projected slice points sorted by lateral position.
  std::vector<ProjPoint> SortedPoints;
  // Robust center position of each lateral bin (meters).
  std::vector<double> Centers;
  // Smoothed per-bin vertical profile values (meters).
  std::vector<double> SmoothedHeights;
  // Representative sample index for each bin (index into SortedPoints).
  std::vector<int> RepresentativeIndices;
};

//-----------------------------------------------------------------------------
// Build one rectangular slice from a trajectory segment and store its local frame.
SliceFrame BuildSlice(const Eigen::Vector3d& p0,
  const Eigen::Vector3d& p1,
  double lateralMin,
  double lateralMax,
  double verticalMin,
  double verticalMax,
  vtkPoints* planePoints,
  vtkCellArray* planePolys)
{
  SliceFrame slice;
  // Center of the current slice, defined as the midpoint of the trajectory segment.
  slice.Center = 0.5 * (p0 + p1);
  // Local slice normal aligned with the trajectory direction.
  slice.Normal = (p1 - p0).normalized();

  // Build a local slice frame:
  //  - vertical direction derived from world-up,
  //  - lateral direction perpendicular to both the trajectory and the vertical.
  const Eigen::Vector3d worldUp = Eigen::Vector3d::UnitZ();
  slice.Vertical = worldUp - worldUp.dot(slice.Normal) * slice.Normal;

  // Normalize the slice directions and orient the lateral axis so that positive values
  // correspond to the physical right side of the trajectory.
  if (slice.Vertical.norm() > 0.0)
  {
    slice.Vertical.normalize();
  }

  slice.Lateral = slice.Normal.cross(slice.Vertical);
  if (slice.Lateral.norm() > 0.0)
  {
    slice.Lateral.normalize();
  }

  if (!planePoints || !planePolys)
  {
    return slice;
  }

  // Build a rectangular slicing window around the trajectory using the lateral
  // and vertical limits provided by the user.
  const vtkIdType baseId = planePoints->GetNumberOfPoints();

  // Center the slicing window vertically according to the configured bounds.
  const double verticalCenter = 0.5 * (verticalMin + verticalMax);
  const double halfVerticalSpan = 0.5 * (verticalMax - verticalMin);
  const Eigen::Vector3d basePoint = slice.Center + verticalCenter * slice.Vertical;

  // Compute the four rectangle corners: left/right and lower/upper bounds.
  const Eigen::Vector3d cornerLeftLower =
    basePoint + lateralMin * slice.Lateral - halfVerticalSpan * slice.Vertical;
  const Eigen::Vector3d cornerRightLower =
    basePoint + lateralMax * slice.Lateral - halfVerticalSpan * slice.Vertical;
  const Eigen::Vector3d cornerRightUpper =
    basePoint + lateralMax * slice.Lateral + halfVerticalSpan * slice.Vertical;
  const Eigen::Vector3d cornerLeftUpper =
    basePoint + lateralMin * slice.Lateral + halfVerticalSpan * slice.Vertical;

  planePoints->InsertNextPoint(cornerLeftLower.data());
  planePoints->InsertNextPoint(cornerRightLower.data());
  planePoints->InsertNextPoint(cornerRightUpper.data());
  planePoints->InsertNextPoint(cornerLeftUpper.data());

  vtkIdType ids[4] = { baseId + 0, baseId + 1, baseId + 2, baseId + 3 };
  planePolys->InsertNextCell(4, ids);

  return slice;
}

//-----------------------------------------------------------------------------
// Project cloud points into each slice and cache in-plane coordinates per slice.
void ProjectPointsToSlices(vtkPolyData* aggregated,
  const std::vector<SliceFrame>& slices,
  double sliceHalfThickness,
  double lateralMin,
  double lateralMax,
  double verticalMin,
  double verticalMax,
  std::vector<std::vector<ProjPoint>>& perSlice,
  vtkPolyData* sliceColoredOut)
{
  perSlice.assign(slices.size(), {});

  vtkPoints* aggPts = aggregated->GetPoints();
  const vtkIdType nAgg = aggPts->GetNumberOfPoints();
  vtkPointData* inPD = aggregated->GetPointData();

  // Optional accumulators for port 1 (sliceColoredOut)
  vtkNew<vtkPoints> slicePtsLR;
  vtkNew<vtkCellArray> sliceVertsLR;
  vtkNew<vtkUnsignedCharArray> colors;
  vtkPointData* outPDLR = nullptr;
  if (sliceColoredOut)
  {
    slicePtsLR->SetDataTypeToDouble();
    colors->SetName("SideColors");
    colors->SetNumberOfComponents(3);
    outPDLR = sliceColoredOut->GetPointData();
    if (outPDLR && inPD)
    {
      outPDLR->CopyAllocate(inPD);
    }
  }

  double point[3];
  for (vtkIdType pid = 0; pid < nAgg; ++pid)
  {
    aggPts->GetPoint(pid, point);
    const Eigen::Vector3d xVec(point[0], point[1], point[2]);
    bool emitted = false; // ensure each input point contributes at most once to port 1

    for (size_t k = 0; k < slices.size(); ++k)
    {
      const auto& slice = slices[k];
      // Vector from slice center to the point
      const Eigen::Vector3d d = xVec - slice.Center;

      // Normal distance to the plane
      const double dn = std::fabs(d.dot(slice.Normal));
      if (dn > sliceHalfThickness)
      {
        continue;
      }
      // In-plane coordinates: lateral (dl) and vertical (dvp)
      const double dl = d.dot(slice.Lateral);
      const double dvp = d.dot(slice.Vertical);

      // Rectangular window bounds in the slice
      if (dl < lateralMin || dl > lateralMax || dvp < verticalMin || dvp > verticalMax)
      {
        continue;
      }
      // Cache projection for downstream per-slice processing (ports 2/3)
      perSlice[k].push_back(ProjPoint{ pid, dl, dvp });

      // Port 1: emit at the first matching slice only (visualization of side by color)
      if (sliceColoredOut && !emitted)
      {
        unsigned char rgb[3] = { 0, 255, 0 }; // right side = green
        if (dl < 0.0)
        {
          rgb[0] = 0;
          rgb[1] = 0;
          rgb[2] = 255;
        } // left side = blue
        vtkIdType nid = slicePtsLR->InsertNextPoint(point);
        sliceVertsLR->InsertNextCell(1);
        sliceVertsLR->InsertCellPoint(nid);
        if (outPDLR && inPD)
        {
          outPDLR->CopyData(inPD, pid, nid);
        }
        colors->InsertNextTuple3(rgb[0], rgb[1], rgb[2]);
        emitted = true;
      }
    }
  }

  if (sliceColoredOut)
  {
    sliceColoredOut->SetPoints(slicePtsLR);
    sliceColoredOut->SetVerts(sliceVertsLR);
    sliceColoredOut->GetPointData()->AddArray(colors);
  }
}

//-----------------------------------------------------------------------------
// Build a median-based lateral profile for one slice.
SliceProfile BuildProfiles(const std::vector<ProjPoint>& slicePoints, int orderBinSize)
{
  SliceProfile profile;
  profile.SortedPoints = slicePoints;
  std::sort(profile.SortedPoints.begin(),
    profile.SortedPoints.end(),
    [](const ProjPoint& a, const ProjPoint& b) { return a.Dl < b.Dl; });

  // Partition the sorted points into consecutive order-bins
  const int n = static_cast<int>(profile.SortedPoints.size());

  // Skip slices that have no points to avoid empty-bin median access
  if (n == 0)
  {
    return profile;
  }

  // Profile parameters: order-based bin size and minimum samples per bin
  const int safeOrderBinSize = std::max(1, orderBinSize);
  const int nbins = (n + safeOrderBinSize - 1) / safeOrderBinSize;
  profile.Centers.reserve(static_cast<size_t>(nbins));
  profile.RepresentativeIndices.reserve(static_cast<size_t>(nbins));
  std::vector<double> medianHeights;
  medianHeights.reserve(static_cast<size_t>(nbins));

  for (int b = 0; b < nbins; ++b)
  {
    const int start = b * safeOrderBinSize;
    const int end = std::min(start + safeOrderBinSize, n);

    // Compute robust bin center (median of dl) and median dvp for the profile
    std::vector<double> dls;
    dls.reserve(static_cast<size_t>(end - start));
    std::vector<double> dvps;
    dvps.reserve(static_cast<size_t>(end - start));
    for (int i = start; i < end; ++i)
    {
      dls.push_back(profile.SortedPoints[static_cast<size_t>(i)].Dl);
      dvps.push_back(profile.SortedPoints[static_cast<size_t>(i)].Dvp);
    }

    const size_t mid = dls.size() / 2;
    std::nth_element(dls.begin(), dls.begin() + static_cast<long>(mid), dls.end());
    std::nth_element(dvps.begin(), dvps.begin() + static_cast<long>(mid), dvps.end());
    profile.Centers.push_back(dls[mid]);
    medianHeights.push_back(dvps[mid]);

    // Find a representative sample in the bin: the point whose dvp is closest to the bin median
    double bestDiff = std::numeric_limits<double>::max();
    int bestIdx = -1;
    for (int i = start; i < end; ++i)
    {
      const double diff = std::fabs(profile.SortedPoints[static_cast<size_t>(i)].Dvp - dvps[mid]);
      if (diff < bestDiff)
      {
        bestDiff = diff;
        bestIdx = i;
      }
    }
    profile.RepresentativeIndices.push_back(bestIdx);
  }

  // Smooth the piecewise-median profile with a 3-point moving average (handles edge bins)
  profile.SmoothedHeights.assign(medianHeights.size(), std::numeric_limits<double>::quiet_NaN());
  for (size_t i = 0; i < medianHeights.size(); ++i)
  {
    double windowSum = medianHeights[i];
    int windowCount = 1;
    if (i > 0)
    {
      windowSum += medianHeights[i - 1];
      ++windowCount;
    }
    if (i + 1 < medianHeights.size())
    {
      windowSum += medianHeights[i + 1];
      ++windowCount;
    }
    profile.SmoothedHeights[i] = windowSum / static_cast<double>(windowCount);
  }

  return profile;
}

//-----------------------------------------------------------------------------
// Emit a profile polyline for visualization output.
void EmitProfile(const SliceFrame& slice,
  const SliceProfile& profile,
  vtkPoints* profilePoints,
  vtkCellArray* profileLines,
  vtkDoubleArray* profileHeights)
{
  std::vector<vtkIdType> ids;
  ids.reserve(profile.SmoothedHeights.size());

  for (size_t i = 0; i < profile.SmoothedHeights.size(); ++i)
  {
    const double dl = profile.Centers[i];
    const double dv = profile.SmoothedHeights[i];
    const Eigen::Vector3d xproj = slice.Center + dl * slice.Lateral + dv * slice.Vertical;
    const vtkIdType pid = profilePoints->InsertNextPoint(xproj.data());
    ids.push_back(pid);
    profileHeights->InsertNextValue(dv);
  }

  if (ids.size() >= 2)
  {
    profileLines->InsertNextCell(static_cast<vtkIdType>(ids.size()));
    for (vtkIdType id : ids)
    {
      profileLines->InsertCellPoint(id);
    }
  }
}

//-----------------------------------------------------------------------------
// Detect vertical jumps between adjacent bins on the smoothed
// profile. If |Δdvp| >= ZChangeMin, pick the representative sample from the steeper bin and
// map it back to 3D.
void DetectCurbs(const SliceFrame& slice,
  const SliceProfile& profile,
  double zChangeMin,
  vtkPolyData* aggregated,
  vtkPoints* outPts,
  vtkCellArray* outVerts,
  vtkUnsignedCharArray* outColors,
  vtkPointData* outPD)
{
  if (profile.SmoothedHeights.size() < 2)
  {
    return;
  }

  vtkPointData* inPD = aggregated->GetPointData();
  for (int binIndex = 1; binIndex < static_cast<int>(profile.SmoothedHeights.size()); ++binIndex)
  {
    const double m0 = profile.SmoothedHeights[static_cast<size_t>(binIndex - 1)];
    const double m1 = profile.SmoothedHeights[static_cast<size_t>(binIndex)];
    if (!std::isnan(m0) && !std::isnan(m1) && std::fabs(m1 - m0) >= zChangeMin)
    {
      const int pickBin = (m1 > m0) ? binIndex : (binIndex - 1);
      const int idx =
        (pickBin >= 0 && pickBin < static_cast<int>(profile.RepresentativeIndices.size()))
        ? profile.RepresentativeIndices[static_cast<size_t>(pickBin)]
        : -1;
      if (idx < 0)
      {
        continue;
      }

      const auto& pp = profile.SortedPoints[static_cast<size_t>(idx)];
      const Eigen::Vector3d xproj = slice.Center + pp.Dl * slice.Lateral + pp.Dvp * slice.Vertical;
      const vtkIdType outIdx = outPts->InsertNextPoint(xproj.data());
      outVerts->InsertNextCell(1);
      outVerts->InsertCellPoint(outIdx);
      if (outPD && inPD)
      {
        outPD->CopyData(inPD, pp.Pid, outIdx);
      }
      if (pp.Dl < 0.0)
      {
        outColors->InsertNextTuple3(0, 0, 255);
      }
      else
      {
        outColors->InsertNextTuple3(0, 255, 0);
      }
    }
  }
}
}

//-----------------------------------------------------------------------------
vtkCurbDetectorOffline::vtkCurbDetectorOffline()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(4);
}

//-----------------------------------------------------------------------------
int vtkCurbDetectorOffline::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCurbDetectorOffline::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* aggregated = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData* trajectory = vtkPolyData::GetData(inputVector[1], 0);

  vtkPolyData* planesOut = vtkPolyData::GetData(outputVector, 0);
  vtkPolyData* sliceColoredOut = vtkPolyData::GetData(outputVector, 1);
  vtkPolyData* curbLROut = vtkPolyData::GetData(outputVector, 2);
  vtkPolyData* curbFromProfileOut = vtkPolyData::GetData(outputVector, 3);

  if (trajectory == nullptr || trajectory->GetPoints() == nullptr ||
    trajectory->GetNumberOfPoints() < 2)
  {
    vtkErrorMacro("Invalid trajectory: need at least 2 poses");
    return 0;
  }

  vtkIdType npts = trajectory->GetNumberOfPoints();
  // Manual slicing dimensions
  if (this->SliceHalfThickness <= 1e-6)
  {
    vtkErrorMacro("SliceHalfThickness must be a positive distance (meters).");
    return 0;
  }

  if (this->SliceLeftDistance < 0.0 || this->SliceRightDistance < 0.0)
  {
    vtkErrorMacro("SliceLeftDistance and SliceRightDistance must be non-negative");
    return 0;
  }

  const double lateralMin = -this->SliceLeftDistance;
  const double lateralMax = this->SliceRightDistance;
  const double verticalMin = this->SliceDownwardDistance;
  const double verticalMax = this->SliceUpwardDistance;

  vtkNew<vtkPoints> planePoints;
  planePoints->SetDataTypeToDouble();
  vtkNew<vtkCellArray> planePolys;

  const int maxSegments = static_cast<int>(std::max<vtkIdType>(0, npts - 1));
  std::vector<SliceFrame> slices;
  slices.reserve(maxSegments);

  for (int i = 0; i < maxSegments; ++i)
  {
    Eigen::Vector3d p0;
    Eigen::Vector3d p1;
    trajectory->GetPoints()->GetPoint(i, p0.data());
    trajectory->GetPoints()->GetPoint(i + 1, p1.data());
    slices.push_back(::BuildSlice(
      p0, p1, lateralMin, lateralMax, verticalMin, verticalMax, planePoints, planePolys));
  }

  if (planesOut)
  {
    planesOut->SetPoints(planePoints);
    planesOut->SetPolys(planePolys);
  }

  if (!(aggregated && aggregated->GetPoints() && !slices.empty()))
  {
    return 1;
  }

  std::vector<std::vector<ProjPoint>> perSlice;
  ::ProjectPointsToSlices(aggregated,
    slices,
    this->SliceHalfThickness,
    lateralMin,
    lateralMax,
    verticalMin,
    verticalMax,
    perSlice,
    sliceColoredOut);

  vtkNew<vtkPoints> profilePoints;
  vtkNew<vtkCellArray> profileLines;
  vtkNew<vtkDoubleArray> profileHeights;
  if (curbLROut)
  {
    profilePoints->SetDataTypeToDouble();
    profileHeights->SetName("ProfileHeight");
    profileHeights->SetNumberOfComponents(1);
  }

  vtkNew<vtkPoints> curbPoints;
  vtkNew<vtkCellArray> curbVerts;
  vtkNew<vtkUnsignedCharArray> curbColors;
  vtkPointData* curbOutPD = nullptr;
  if (curbFromProfileOut)
  {
    curbPoints->SetDataTypeToDouble();
    curbColors->SetName("SideColors");
    curbColors->SetNumberOfComponents(3);
    curbOutPD = curbFromProfileOut->GetPointData();
    vtkPointData* inPD = aggregated->GetPointData();
    if (curbOutPD && inPD)
    {
      curbOutPD->CopyAllocate(inPD);
    }
  }

  const int orderBinSize = std::max(1, this->OrderBinSize);
  for (size_t k = 0; k < slices.size(); ++k)
  {
    const SliceProfile profile = ::BuildProfiles(perSlice[k], orderBinSize);
    if (profile.SortedPoints.empty())
    {
      continue;
    }

    if (curbLROut)
    {
      ::EmitProfile(slices[k], profile, profilePoints, profileLines, profileHeights);
    }
    if (curbFromProfileOut)
    {
      ::DetectCurbs(slices[k],
        profile,
        this->ZChangeMin,
        aggregated,
        curbPoints,
        curbVerts,
        curbColors,
        curbOutPD);
    }
  }

  if (curbLROut)
  {
    curbLROut->SetPoints(profilePoints);
    curbLROut->SetLines(profileLines);
    curbLROut->GetPointData()->AddArray(profileHeights);
  }

  if (curbFromProfileOut)
  {
    curbFromProfileOut->SetPoints(curbPoints);
    curbFromProfileOut->SetVerts(curbVerts);
    curbFromProfileOut->GetPointData()->AddArray(curbColors);
  }

  return 1;
}
