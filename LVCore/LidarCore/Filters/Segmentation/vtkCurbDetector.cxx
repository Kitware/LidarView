/*=========================================================================

  Program:   LidarView
  Module:    vtkCurbDetector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCurbDetector.h"

#include <vtkCellArray.h>
#include <vtkCubeSource.h>
#include <vtkDataArray.h>
#include <vtkDataSetAttributes.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

vtkStandardNewMacro(vtkCurbDetector);
namespace
{
// 3D line RANSAC on a subset of points. Returns inlier point ids.
std::vector<vtkIdType> RansacLine3D(vtkPoints* points,
  const std::vector<vtkIdType>& pointIds,
  int maxIterations,
  double distanceThreshold,
  unsigned int seed)
{
  std::vector<vtkIdType> inliers;
  if (!points || pointIds.size() < 2)
  {
    return inliers;
  }

  std::vector<std::array<double, 3>> points3D;
  points3D.reserve(pointIds.size());
  double coords[3];
  for (vtkIdType fid : pointIds)
  {
    points->GetPoint(fid, coords);
    points3D.push_back({ coords[0], coords[1], coords[2] });
  }

  std::mt19937 randomEngine(seed);
  std::uniform_int_distribution<size_t> uniformIndex(0, points3D.size() - 1);

  const double distanceThresholdSquared = distanceThreshold * distanceThreshold;
  size_t bestInlierCount = 0;
  std::vector<size_t> bestInlierIndices;

  for (int iteration = 0; iteration < maxIterations; ++iteration)
  {
    size_t sampleIndexA = uniformIndex(randomEngine);
    size_t sampleIndexB = uniformIndex(randomEngine);
    if (sampleIndexA == sampleIndexB)
    {
      continue;
    }

    const auto& pointA = points3D[sampleIndexA];
    const auto& pointB = points3D[sampleIndexB];
    double dirX = pointB[0] - pointA[0];
    double dirY = pointB[1] - pointA[1];
    double dirZ = pointB[2] - pointA[2];
    double directionNormSquared = dirX * dirX + dirY * dirY + dirZ * dirZ;
    if (directionNormSquared < 1e-8)
    {
      continue;
    }
    double directionNorm = std::sqrt(directionNormSquared);
    double unitX = dirX / directionNorm;
    double unitY = dirY / directionNorm;
    double unitZ = dirZ / directionNorm;

    std::vector<size_t> inlierIndices;
    inlierIndices.reserve(points3D.size());
    for (size_t pointIndex = 0; pointIndex < points3D.size(); ++pointIndex)
    {
      double deltaX = points3D[pointIndex][0] - pointA[0];
      double deltaY = points3D[pointIndex][1] - pointA[1];
      double deltaZ = points3D[pointIndex][2] - pointA[2];
      double deltaSquared = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
      double projection = deltaX * unitX + deltaY * unitY + deltaZ * unitZ;
      double distanceSquared = deltaSquared - projection * projection;
      if (distanceSquared <= distanceThresholdSquared)
      {
        inlierIndices.push_back(pointIndex);
      }
    }

    if (inlierIndices.size() > bestInlierCount)
    {
      bestInlierCount = inlierIndices.size();
      bestInlierIndices.swap(inlierIndices);
    }
  }

  inliers.reserve(bestInlierIndices.size());
  for (size_t idx : bestInlierIndices)
  {
    inliers.push_back(pointIds[idx]);
  }
  return inliers;
}
}

//-----------------------------------------------------------------------------
vtkCurbDetector::vtkCurbDetector()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
int vtkCurbDetector::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input || !input->GetPoints() || input->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro("Invalid or empty input");
    return 0;
  }

  vtkIdType inputPointCount = input->GetNumberOfPoints();
  vtkPoints* inputPoints = input->GetPoints();

  // Derive ROI bounds from interactive pose: min-corner=BoxPosition, lengths=BoxScale
  const double lx = std::max(1e-6, this->BoxScale[0]);
  const double ly = std::max(1e-6, this->BoxScale[1]);
  const double lz = std::max(1e-6, this->BoxScale[2]);
  const double xMin = this->BoxPosition[0];
  const double yMin = this->BoxPosition[1];
  const double zMin = this->BoxPosition[2];
  const double xMax = xMin + lx;
  const double yMax = yMin + ly;
  const double zMax = zMin + lz;

  if (xMin >= xMax || yMin >= yMax || zMin >= zMax)
  {
    vtkErrorMacro(
      "Invalid ROI bounds: minimun value must be smaller than maximun value on all axes");
    return 0;
  }

  // Build selection from input points that lie inside ROI (Interactive Box)
  double pointCoords[3];
  std::vector<vtkIdType> selected;
  selected.reserve(static_cast<size_t>(inputPointCount / 2));
  for (vtkIdType pointId = 0; pointId < inputPointCount; ++pointId)
  {
    inputPoints->GetPoint(pointId, pointCoords);
    if (pointCoords[0] >= xMin && pointCoords[0] <= xMax &&
      pointCoords[1] >= yMin && pointCoords[1] <= yMax &&
      pointCoords[2] >= zMin && pointCoords[2] <= zMax)
    {
      selected.push_back(pointId);
    }
  }

  // Port 0: ROI-filtered points
  vtkPolyData* output = vtkPolyData::GetData(outputVector->GetInformationObject(0));
  vtkNew<vtkPoints> filteredOutputPoints;
  filteredOutputPoints->SetDataTypeToFloat();
  filteredOutputPoints->SetNumberOfPoints(static_cast<vtkIdType>(selected.size()));

  // Set points, copy point-data, and build vertex cells
  vtkNew<vtkCellArray> filteredVerts;
  filteredVerts->AllocateEstimate(static_cast<vtkIdType>(selected.size()), 1);
  vtkPointData* inPD_init = input->GetPointData();
  vtkPointData* outPD_init = output->GetPointData();
  outPD_init->CopyAllocate(inPD_init, static_cast<vtkIdType>(selected.size()));
  double selectedPointCoords[3];
  for (size_t i = 0; i < selected.size(); ++i)
  {
    const vtkIdType inId = selected[i];
    const vtkIdType outId = static_cast<vtkIdType>(i);
    inputPoints->GetPoint(inId, selectedPointCoords);
    filteredOutputPoints->SetPoint(outId, selectedPointCoords);
    outPD_init->CopyData(inPD_init, inId, outId);
    filteredVerts->InsertNextCell(1);
    filteredVerts->InsertCellPoint(outId);
  }

  output->SetPoints(filteredOutputPoints);
  output->SetVerts(filteredVerts);

  // Use ROI-filtered points from port 0 for curb detection
  vtkPoints* filteredPoints = output->GetPoints();
  vtkPointData* filteredPointData = output->GetPointData();
  vtkDataArray* ringArr = filteredPointData ? filteredPointData->GetArray("laser_id") : nullptr;
  vtkDataArray* timestampArr = filteredPointData ? filteredPointData->GetArray("timestamp") : nullptr;
  if (filteredPoints && filteredPointData && ringArr)
  {
    vtkIdType filteredPointCount = filteredPoints->GetNumberOfPoints();
    double ringRange[2];
    ringArr->GetRange(ringRange);
    int maxRing = static_cast<int>(ringRange[1]);
    if (maxRing >= 0 && filteredPointCount > 0)
    {
      // Group ROI-filtered points by laser ring
      std::vector<std::vector<vtkIdType>> rings(static_cast<size_t>(maxRing) + 1);
      for (vtkIdType fid = 0; fid < filteredPointCount; ++fid)
      {
        int rid = static_cast<int>(ringArr->GetComponent(fid, 0));
        rings[static_cast<size_t>(rid)].push_back(fid);
      }

      // Candidate curb edge point indices detected from Z discontinuities within each laser ring
      std::vector<vtkIdType> edgeIds;

      // Precompute frame duration in microseconds from spin rate
      const bool useTimeGating = (timestampArr != nullptr && this->SpinRateHz > 1e-9);
      const double frameDurationUs = useTimeGating ? (1.0e6 / this->SpinRateHz) : 0.0;
      const double maxAllowedGapUs = useTimeGating ? (0.5 * frameDurationUs) : 0.0;

      // Iterate over each laser ring to process points ring by ring
      for (int ringIndex = 0; ringIndex <= maxRing; ++ringIndex)
      {
        auto& idxsAll = rings[static_cast<size_t>(ringIndex)];

        // Cache coordinates for this ring
        std::vector<double> xCoords(idxsAll.size());
        std::vector<double> yCoords(idxsAll.size());
        std::vector<double> zCoords(idxsAll.size());

        std::vector<double> tStamps; // microseconds
        if (useTimeGating)
        {
          tStamps.resize(idxsAll.size());
        }
        for (size_t pointOrderIndex = 0; pointOrderIndex < idxsAll.size(); ++pointOrderIndex)
        {
          filteredPoints->GetPoint(idxsAll[pointOrderIndex], pointCoords);
          xCoords[pointOrderIndex] = pointCoords[0];
          yCoords[pointOrderIndex] = pointCoords[1];
          zCoords[pointOrderIndex] = pointCoords[2];
          if (useTimeGating)
          {
            tStamps[pointOrderIndex] = timestampArr->GetComponent(idxsAll[pointOrderIndex], 0);
          }
        }

        // Build an index order that sorts points of this ring by X coordinate
        std::vector<size_t> orderByX;
        orderByX.resize(idxsAll.size());
        std::iota(orderByX.begin(), orderByX.end(), 0);
        std::sort(orderByX.begin(),
          orderByX.end(),
          [&](size_t a, size_t b) { return xCoords[a] < xCoords[b]; });

        // Number of neighboring points on each side of the center point discontinuities used to
        // evaluate Z (clamp to at least 1)
        const int neighborCount = std::max(1, this->NeighborCount);

        // Require enough points for a symmetric window
        if (orderByX.size() < static_cast<size_t>(2 * neighborCount + 1))
        {
          continue;
        }

        // Detect curb edge candidates by sliding a symmetric window along X and measuring Z height
        // jumps within each laser ring
        for (size_t centerIndex = neighborCount; centerIndex + neighborCount < orderByX.size();
             ++centerIndex)
        {
          double sumDeltaZ = 0.0;
          int contributingPairs = 0;
          for (int neighborOffset = 1; neighborOffset <= neighborCount; ++neighborOffset)
          {
            size_t indexA = orderByX[centerIndex - neighborOffset];
            size_t indexB = orderByX[centerIndex + neighborOffset];
            // If timestamps are detected, skip pairs that likely cross a frame boundary
            if (useTimeGating)
            {
              double dtUs = std::fabs(tStamps[indexB] - tStamps[indexA]);
              if (dtUs > maxAllowedGapUs)
              {
                continue; // ignore discontinuous temporal pairs
              }
            }
            sumDeltaZ += std::fabs(zCoords[indexB] - zCoords[indexA]);
            ++contributingPairs;
          }
          // Normalize by contributing pairs so the threshold is an average ΔZ per valid pair
          if (contributingPairs > 0 &&
            (sumDeltaZ / static_cast<double>(contributingPairs)) >= this->ZChangeMin)
          {
            edgeIds.push_back(idxsAll[orderByX[centerIndex]]);
          }
        }
      }

      // Sort and remove duplicate edge candidate point indices
      std::sort(edgeIds.begin(), edgeIds.end());
      edgeIds.erase(std::unique(edgeIds.begin(), edgeIds.end()), edgeIds.end());

      // Split edge candidate points into left and right sets based on the sign of X coordinate
      std::vector<vtkIdType> leftFids;
      leftFids.reserve(edgeIds.size());
      std::vector<vtkIdType> rightFids;
      rightFids.reserve(edgeIds.size());
      for (auto fid : edgeIds)
      {
        double edgePointCoords[3];
        filteredPoints->GetPoint(fid, edgePointCoords);
        if (edgePointCoords[0] < 0.0)
        {
          leftFids.push_back(fid);
        }

        else if (edgePointCoords[0] > 0.0)
        {
          rightFids.push_back(fid);
        }
      }

      // Apply 3D line RANSAC separately on left and right edge candidates
      std::vector<vtkIdType> inliersLeft = RansacLine3D(filteredPoints,
        leftFids,
        /*maxIterations*/ 300,
        /*distanceThreshold*/ std::max(1e-6, this->RansacDistanceThreshold),
        /*seed*/ 5489u);
      std::vector<vtkIdType> inliersRight = RansacLine3D(filteredPoints,
        rightFids,
        /*maxIterations*/ 300,
        /*distanceThreshold*/ std::max(1e-6, this->RansacDistanceThreshold),
        /*seed*/ 5489u);

      // Port 1: curb inliers (left=blue, right=green)
      vtkPolyData* curbOut = vtkPolyData::GetData(outputVector->GetInformationObject(1));
      if (curbOut)
      {
        // Merge left and right RANSAC inliers into a single curb point index list
        std::vector<vtkIdType> allFids;
        allFids.reserve(inliersLeft.size() + inliersRight.size());
        allFids.insert(allFids.end(), inliersLeft.begin(), inliersLeft.end());
        allFids.insert(allFids.end(), inliersRight.begin(), inliersRight.end());

        vtkNew<vtkPoints> curbPoints;
        curbPoints->SetDataTypeToFloat();
        curbPoints->SetNumberOfPoints(static_cast<vtkIdType>(allFids.size()));
        curbOut->SetPoints(curbPoints);

        vtkNew<vtkCellArray> verts;
        verts->AllocateEstimate(static_cast<vtkIdType>(allFids.size()), 1);

        vtkPointData* inPD_full = input->GetPointData();
        vtkPointData* outPD_curb = curbOut->GetPointData();
        outPD_curb->CopyAllocate(inPD_full, static_cast<vtkIdType>(allFids.size()));

        // Sort inlier indices to allow binary_search when assigning left and right curb
        std::sort(inliersLeft.begin(), inliersLeft.end());
        std::sort(inliersRight.begin(), inliersRight.end());

        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetName("CurbSideColors");
        colors->SetNumberOfComponents(3);
        colors->SetNumberOfTuples(static_cast<vtkIdType>(allFids.size()));

        double inputPointCoords[3];
        for (vtkIdType curbIndex = 0; curbIndex < static_cast<vtkIdType>(allFids.size());
             ++curbIndex)
        {
          vtkIdType fid = allFids[static_cast<size_t>(curbIndex)];
          vtkIdType inid = selected[static_cast<size_t>(fid)];

          input->GetPoints()->GetPoint(inid, inputPointCoords);
          curbPoints->SetPoint(curbIndex, inputPointCoords);
          outPD_curb->CopyData(inPD_full, inid, curbIndex);

          verts->InsertNextCell(1);
          verts->InsertCellPoint(curbIndex);

          // Assign per-point color based on curb side: left = blue, right = green
          if (std::binary_search(inliersLeft.begin(), inliersLeft.end(), fid))
          {
            colors->SetTuple3(curbIndex, 0, 0, 255);
          }
          else
          {
            colors->SetTuple3(curbIndex, 0, 255, 0);
          }
        }
        curbOut->SetVerts(verts);
        outPD_curb->AddArray(colors);
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkCurbDetector::SetBoxScale(double sx, double sy, double sz)
{
  sx = std::fabs(sx);
  sy = std::fabs(sy);
  sz = std::fabs(sz);
  if (this->BoxScale[0] == sx && this->BoxScale[1] == sy && this->BoxScale[2] == sz)
  {
    return;
  }
  this->BoxScale[0] = sx;
  this->BoxScale[1] = sy;
  this->BoxScale[2] = sz;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCurbDetector::SetBoxScale(const double s[3])
{
  if (!s)
  {
    return;
  }
  this->SetBoxScale(s[0], s[1], s[2]);
}
