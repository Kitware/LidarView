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

  double lateralMin = -this->SliceLeftDistance;
  double lateralMax = this->SliceRightDistance;
  double verticalMin = this->SliceDownwardDistance;
  double verticalMax = this->SliceUpwardDistance;

  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  vtkNew<vtkCellArray> polys;

  // Keep plane parameters for slicing aggregated cloud later
  std::vector<Eigen::Vector3d> planeCenters;
  std::vector<Eigen::Vector3d> planeNormals;
  std::vector<Eigen::Vector3d> planeL;
  std::vector<Eigen::Vector3d> planeV;

  // Use all available consecutive segments along trajectory
  const int maxSegments = static_cast<int>(std::max<vtkIdType>(0, npts - 1));
  planeCenters.reserve(maxSegments);
  planeNormals.reserve(maxSegments);
  planeL.reserve(maxSegments);
  planeV.reserve(maxSegments);

  for (int i = 0; i < maxSegments; ++i)
  {
    Eigen::Vector3d p0, p1;
    trajectory->GetPoints()->GetPoint(i, p0.data());
    trajectory->GetPoints()->GetPoint(i + 1, p1.data());

    // Center of the current slice, defined as the midpoint of the trajectory segment.
    const Eigen::Vector3d sliceCenter = 0.5 * (p0 + p1);

    // Local slice normal aligned with the trajectory direction.
    Eigen::Vector3d sliceNormal = (p1 - p0).normalized();

    // Build a local slice frame:
    //  - vertical direction derived from world-up,
    //  - lateral direction perpendicular to both the trajectory and the vertical.
    const Eigen::Vector3d worldUp = Eigen::Vector3d::UnitZ();
    Eigen::Vector3d axisVertical = worldUp - worldUp.dot(sliceNormal) * sliceNormal;

    // Normalize the slice directions and orient the lateral axis so that positive values
    // correspond to the physical right side of the trajectory.
    if (axisVertical.norm() > 0.0)
    {
      axisVertical.normalize();
    }
    Eigen::Vector3d axisLateral = sliceNormal.cross(axisVertical);
    if (axisLateral.norm() > 0.0)
    {
      axisLateral.normalize();
    }

    // Build a rectangular slicing window around the trajectory using the lateral
    // and vertical limits provided by the user.
    vtkIdType baseId = pts->GetNumberOfPoints();
    // Center the slicing window vertically according to the configured bounds.
    const double verticalCenter = 0.5 * (verticalMin + verticalMax);
    const double halfVerticalSpan = 0.5 * (verticalMax - verticalMin);

    const Eigen::Vector3d basePoint = sliceCenter + verticalCenter * axisVertical;

    // Compute the four rectangle corners: left/right and lower/upper bounds.
    const Eigen::Vector3d cornerLeftLower =
      basePoint + lateralMin * axisLateral - halfVerticalSpan * axisVertical;

    const Eigen::Vector3d cornerRightLower =
      basePoint + lateralMax * axisLateral - halfVerticalSpan * axisVertical;

    const Eigen::Vector3d cornerRightUpper =
      basePoint + lateralMax * axisLateral + halfVerticalSpan * axisVertical;

    const Eigen::Vector3d cornerLeftUpper =
      basePoint + lateralMin * axisLateral + halfVerticalSpan * axisVertical;

    pts->InsertNextPoint(cornerLeftLower.data());
    pts->InsertNextPoint(cornerRightLower.data());
    pts->InsertNextPoint(cornerRightUpper.data());
    pts->InsertNextPoint(cornerLeftUpper.data());

    vtkIdType ids[4] = { baseId + 0, baseId + 1, baseId + 2, baseId + 3 };
    polys->InsertNextCell(4, ids);

    // Store slice geometry for later point projection and profile extraction.
    planeCenters.push_back(sliceCenter);
    planeNormals.push_back(sliceNormal);
    planeL.push_back(axisLateral);
    planeV.push_back(axisVertical);
  }

  if (planesOut)
  {
    planesOut->SetPoints(pts);
    planesOut->SetPolys(polys);
  }

  if (aggregated && aggregated->GetPoints() && !planeCenters.empty())
  {
    vtkPoints* aggPts = aggregated->GetPoints();
    const vtkIdType nAgg = aggPts->GetNumberOfPoints();
    vtkPointData* inPD2 = aggregated->GetPointData();

    // Shared per-slice projection cache (dl along lateral l, dvp along vertical v)
    struct ProjPoint
    {
      vtkIdType pid;
      double dl;
      double dvp;
    };
    std::vector<std::vector<ProjPoint>> perSlice(planeCenters.size());

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
      if (outPDLR && inPD2)
      {
        outPDLR->CopyAllocate(inPD2);
      }
    }

    double point[3];
    for (vtkIdType pid = 0; pid < nAgg; ++pid)
    {
      aggPts->GetPoint(pid, point);
      const Eigen::Vector3d xVec(point[0], point[1], point[2]);
      bool emitted = false; // ensure each input point contributes at most once to port 1
      for (size_t k = 0; k < planeCenters.size(); ++k)
      {
        // Slice frame for this segment: center, normal, lateral axis, vertical axis
        const auto& sliceCenter = planeCenters[k];
        const auto& planeNormal = planeNormals[k];
        const auto& lateralAxis = planeL[k];
        const auto& verticalAxis = planeV[k];

        // Vector from slice center to the point
        const Eigen::Vector3d d = xVec - sliceCenter;

        // Normal distance to the plane
        const double dn = std::fabs(d.dot(planeNormal));
        if (dn > this->SliceHalfThickness)
        {
          continue;
        }

        // In-plane coordinates: lateral (dl) and vertical (dvp)
        const double dl = d.dot(lateralAxis);
        const double dvp = d.dot(verticalAxis);

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
          if (outPDLR && inPD2)
          {
            outPDLR->CopyData(inPD2, pid, nid);
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

    // Build a 1D lateral profile once per slice and emit ports 2 (profile) and 3 (candidates)
    vtkNew<vtkPoints> profPts;
    vtkNew<vtkCellArray> profLines;
    vtkNew<vtkDoubleArray> profHeights;
    if (curbLROut)
    {
      profPts->SetDataTypeToDouble();
      profHeights->SetName("ProfileHeight");
      profHeights->SetNumberOfComponents(1);
    }
    vtkNew<vtkPoints> outPts;
    vtkNew<vtkCellArray> outVerts;
    vtkNew<vtkUnsignedCharArray> outColors;
    vtkPointData* outPD = nullptr;
    if (curbFromProfileOut)
    {
      outPts->SetDataTypeToDouble();
      outColors->SetName("SideColors");
      outColors->SetNumberOfComponents(3);
      outPD = curbFromProfileOut->GetPointData();
      if (outPD && inPD2)
      {
        outPD->CopyAllocate(inPD2);
      }
    }

    // Profile parameters: order-based bin size and minimum samples per bin
    const int orderBinSize = 20;  // target number of points per bin along sorted dl
    for (size_t k = 0; k < planeCenters.size(); ++k)
    {
      auto sliceProjPoints = perSlice[k];
      std::sort(sliceProjPoints.begin(),
        sliceProjPoints.end(),
        [](const ProjPoint& a, const ProjPoint& b) { return a.dl < b.dl; });

      const auto& sliceCenter = planeCenters[k];
      const auto& lateralAxis = planeL[k];
      const auto& verticalAxis = planeV[k];

      // Partition the sorted points into consecutive order-bins
      const int n = static_cast<int>(sliceProjPoints.size());
      // Skip slices that have no points to avoid empty-bin median access
      if (n == 0)
      {
        continue;
      }
      const int nbins = (n + orderBinSize - 1) / orderBinSize;
      std::vector<double> centers;
      centers.reserve(static_cast<size_t>(nbins));
      std::vector<double> med;
      med.reserve(static_cast<size_t>(nbins));
      std::vector<int> reprIdx;
      reprIdx.reserve(static_cast<size_t>(nbins));
      for (int b = 0; b < nbins; ++b)
      {
        const int start = b * orderBinSize;
        const int end = std::min(start + orderBinSize, n);
        // Compute robust bin center (median of dl) and median dvp for the profile
        std::vector<double> dls;
        dls.reserve(static_cast<size_t>(end - start));
        std::vector<double> dvps;
        dvps.reserve(static_cast<size_t>(end - start));
        for (int i = start; i < end; ++i)
        {
          dls.push_back(sliceProjPoints[static_cast<size_t>(i)].dl);
          dvps.push_back(sliceProjPoints[static_cast<size_t>(i)].dvp);
        }
        size_t mid = dls.size() / 2;
        std::nth_element(dls.begin(), dls.begin() + mid, dls.end());
        std::nth_element(dvps.begin(), dvps.begin() + mid, dvps.end());
        centers.push_back(dls[mid]);
        med.push_back(dvps[mid]);

        // Find a representative sample in the bin: the point whose dvp is closest to the bin median
        double bestDiff = std::numeric_limits<double>::max();
        int bestIdx = -1;
        for (int i = start; i < end; ++i)
        {
          const double diff = std::fabs(sliceProjPoints[static_cast<size_t>(i)].dvp - dvps[mid]);
          if (diff < bestDiff)
          {
            bestDiff = diff;
            bestIdx = i;
          }
        }
        reprIdx.push_back(bestIdx);
      }
      // Smooth the piecewise-median profile with a 3-point moving average (handles edge bins)
      std::vector<double> smooth;
      smooth.assign(med.size(), std::numeric_limits<double>::quiet_NaN());
      for (size_t i = 0; i < med.size(); ++i)
      {
        double windowSum = med[i];
        int windowCount = 1;
        if (i > 0)
        {
          windowSum += med[i - 1];
          ++windowCount;
        }
        if (i + 1 < med.size())
        {
          windowSum += med[i + 1];
          ++windowCount;
        }
        smooth[i] = windowSum / static_cast<double>(windowCount);
      }

      // Emit port 2 (profile): reconstruct 3D points from (centers[i], smooth[i]) on the slice
      // plane
      if (curbLROut)
      {
        std::vector<vtkIdType> ids;
        ids.reserve(smooth.size());
        for (size_t i = 0; i < smooth.size(); ++i)
        {
          const double dlc = centers[i];
          const double dv = smooth[i];
          const Eigen::Vector3d xproj = sliceCenter + dlc * lateralAxis + dv * verticalAxis;
          vtkIdType pid = profPts->InsertNextPoint(xproj.data());
          ids.push_back(pid);
          profHeights->InsertNextValue(dv);
        }
        if (ids.size() >= 2)
        {
          profLines->InsertNextCell(static_cast<vtkIdType>(ids.size()));
          for (vtkIdType id : ids)
          {
            profLines->InsertCellPoint(id);
          }
        }
      }

      // Emit port 3 (candidates): detect vertical jumps between adjacent bins on the smoothed
      // profile. If |Δdvp| >= ZChangeMin, pick the representative sample from the steeper bin and
      // map it back to 3D.
      if (curbFromProfileOut && smooth.size() >= 2)
      {
        for (int binIndex = 1; binIndex < static_cast<int>(smooth.size()); ++binIndex)
        {
          const double m0 = smooth[static_cast<size_t>(binIndex - 1)];
          const double m1 = smooth[static_cast<size_t>(binIndex)];
          if (!std::isnan(m0) && !std::isnan(m1) && std::fabs(m1 - m0) >= this->ZChangeMin)
          {
            const int pickBin = (m1 > m0) ? binIndex : (binIndex - 1);
            const int idx = (pickBin >= 0 && pickBin < static_cast<int>(reprIdx.size()))
              ? reprIdx[static_cast<size_t>(pickBin)]
              : -1;
            if (idx < 0)
            {
              continue;
            }
            const auto& pp = sliceProjPoints[static_cast<size_t>(idx)];
            const Eigen::Vector3d xproj = sliceCenter + pp.dl * lateralAxis + pp.dvp * verticalAxis;
            vtkIdType outIdx = outPts->InsertNextPoint(xproj.data());
            outVerts->InsertNextCell(1);
            outVerts->InsertCellPoint(outIdx);
            if (outPD && inPD2)
            {
              outPD->CopyData(inPD2, pp.pid, outIdx);
            }
            if (pp.dl < 0.0)
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
    curbLROut->SetPoints(profPts);
    curbLROut->SetLines(profLines);
    curbLROut->GetPointData()->AddArray(profHeights);

    curbFromProfileOut->SetPoints(outPts);
    curbFromProfileOut->SetVerts(outVerts);
    curbFromProfileOut->GetPointData()->AddArray(outColors);
  }

  return 1;
}
