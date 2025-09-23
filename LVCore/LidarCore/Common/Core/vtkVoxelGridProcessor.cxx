/*=========================================================================

  Program:   LidarView
  Module:    vtkVoxelGridProcessor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// STL includes
#include <limits>

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkCellArray.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

// Local includes
#include "vtkVoxelGridProcessor.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVoxelGridProcessor)

//------------------------------------------------------------------------------
vtkVoxelGridProcessor::vtkVoxelGridProcessor()
{
  this->UpdateVoxelSize();
}

//------------------------------------------------------------------------------
bool vtkVoxelGridProcessor::AddPoint(vtkDataSet* points, vtkIdType id, const double coord[3])
{
  if (!this->IsVoxelGridValid)
  {
    vtkWarningMacro("vtkVoxelGridProcessor::Add : invalid voxel grid (point not added)");
    return false;
  }
  if (points->GetNumberOfPoints() < id)
  {
    vtkWarningMacro("vtkVoxelGridProcessor::Add : invalid point id");
    return false;
  }

  // Initialize the point data to keep the same structure as the input
  if (!this->VoxelPointDataInitialized)
  {
    this->InitializeData();
    this->Output->GetPointData()->CopyAllocate(points->GetPointData());
    this->VoxelPointDataInitialized = true;
  }

  Eigen::Vector3d position;
  position << coord[0], coord[1], coord[2];
  double* pointCoord = position.data();

  if (!IsInBounds(position))
  {
    return true;
  }

  // Compute the voxel id
  uint64 voxelId = this->PositionToVoxelId(position);

  // Get the number of instances of the voxel in the grid (0 or 1)
  int nbInstancesInVox = this->VoxelGrid.count(voxelId);
  vtkIdType pointId = -1;
  if (nbInstancesInVox > 0)
  {
    // Get the point id in the voxel grid if the voxel is already in the grid
    pointId = this->VoxelGrid[voxelId];
  }

  // Add the point to the voxel grid
  bool addPoint = false;
  // Add a new point if the voxel is not already in the grid
  bool newPoint = false;

  switch (this->Sampling)
  {
    case SamplingMode::FIRST:
    {
      switch (nbInstancesInVox)
      {
        case 0:
          addPoint = true;
          newPoint = true;
          break;

        default:
          break;
      }
      break;
    }

    case SamplingMode::LAST:
    {
      addPoint = true;
      switch (nbInstancesInVox)
      {
        case 0:
          newPoint = true;
          break;

        default:
          break;
      }
      break;
    }

    case SamplingMode::MAX_INTENSITY:
    {
      // Add the current point if the voxel is not already in the grid
      addPoint = (nbInstancesInVox == 0);
      newPoint = addPoint;
      if (newPoint)
      {
        break;
      }
      vtkDataArray* intensityArray =
        vtkDataArray::SafeDownCast(this->Output->GetPointData()->GetAbstractArray("intensity"));
      vtkDataArray* newIntensityArray =
        vtkDataArray::SafeDownCast(points->GetPointData()->GetAbstractArray("intensity"));
      if (intensityArray && newIntensityArray)
      {
        float intensity = intensityArray->GetTuple1(pointId);
        float newIntensity = newIntensityArray->GetTuple1(id);
        if (newIntensity > intensity)
        {
          addPoint = true;
        }
      }
      else
      {
        vtkWarningMacro("vtkVoxelGridProcessor::Add : no intensity array");
        return false;
      }
      break;
    }

    case SamplingMode::CENTER_POINT:
    {
      switch (nbInstancesInVox)
      {
        case 0:
        {
          addPoint = true;
          newPoint = true;
          // Set the point coordinates to the voxel center
          position = this->VoxelIdToPosition(voxelId);
          pointCoord = position.data();
          break;
        }
        default:
        {
          break;
        }
      }
      break;
    }

    case SamplingMode::CENTROID:
    {
      addPoint = true;
      switch (nbInstancesInVox)
      {
        case 0:
        {
          newPoint = true;
          this->NumberOfPointsPerVoxel.push_back(1);
          break;
        }

        default:
        {
          // Security check to avoid division by 0 in case of sampling mode change during the
          // process
          if (pointId < 0 || this->NumberOfPointsPerVoxel.size() <= static_cast<size_t>(pointId))
          {
            vtkErrorMacro("vtkVoxelGridProcessor::Add : CENTROID sampling mode requires to clear "
                          "the voxel grid before adding points");
            return false;
          }
          double previousCoord[3];
          this->Output->GetPoints()->GetPoint(pointId, previousCoord);
          int nbPtsInVox = this->NumberOfPointsPerVoxel[pointId];
          // Update the centroid
          pointCoord[0] = (previousCoord[0] * nbPtsInVox + pointCoord[0]) / (nbPtsInVox + 1);
          pointCoord[1] = (previousCoord[1] * nbPtsInVox + pointCoord[1]) / (nbPtsInVox + 1);
          pointCoord[2] = (previousCoord[2] * nbPtsInVox + pointCoord[2]) / (nbPtsInVox + 1);
          this->NumberOfPointsPerVoxel[pointId]++;
          break;
        }
      }
      break;
    }

    default:
      vtkWarningMacro("vtkVoxelGridProcessor::AddPoint : invalid sampling mode, point not added");
      break;
  }

  // Add the point to the voxel grid
  if (addPoint)
  {
    // If the voxel is not already in the grid, add it and insert the point
    if (newPoint)
    {
      // Output may not have enough memory allocated, the data needs to be resized
      if (!this->ResizeData())
      {
        vtkWarningMacro("vtkVoxelGridProcessor::Add : resize data failed");
        return false;
      }
      // Insert the current point in the voxel grid
      this->Output->GetPoints()->InsertNextPoint(pointCoord);
      this->Output->GetVerts()->InsertNextCell(1, &this->VoxelCount);
      this->Output->GetPointData()->CopyData(points->GetPointData(), this->VoxelCount, 1, id);
      this->VoxelGrid[voxelId] = this->VoxelCount;
      this->VoxelCount++;
    }
    else
    {
      // Update the point coordinates and data in an existing voxel
      this->Output->GetPoints()->SetPoint(pointId, pointCoord);
      this->Output->GetPointData()->CopyData(points->GetPointData(), pointId, 1, id);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkVoxelGridProcessor::AddPoint(vtkDataSet* points, vtkIdType id)
{
  double coord[3];
  points->GetPoint(id, coord);
  return this->AddPoint(points, id, coord);
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::AddPoints(vtkDataSet* points)
{
  for (int idx = 0; idx < points->GetPoints()->GetNumberOfPoints(); ++idx)
  {
    this->AddPoint(points, idx);
  }
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::UpdateVoxelSize()
{
  // Check if the bounds are valid
  double diff[3];
  for (int i = 0; i < 3; i++)
  {
    diff[i] = std::ceil((this->Bounds[2 * i + 1] - this->Bounds[2 * i]) / this->LeafSize);
    // Bounds must be correctly oriented
    if (diff[i] <= 0.0)
    {
      this->IsVoxelGridValid = false;
      vtkErrorMacro("vtkVoxelGridProcessor::UpdateVoxelSize : invalid bounds");
      return;
    }
  }

  // Compute the grid size in each direction
  uint64 sizeX = static_cast<uint64>(diff[0]);
  uint64 sizeY = static_cast<uint64>(diff[1]);
  uint64 sizeZ = static_cast<uint64>(diff[2]);
  // Check for overflow
  if (ULLONG_MAX / sizeX < sizeY || ULLONG_MAX / (sizeX * sizeY) < sizeZ)
  {
    this->IsVoxelGridValid = false;
    vtkErrorMacro("vtkVoxelGridProcessor::UpdateVoxelSize : voxel grid size overflow, increase the "
                  "leaf size or reduce the bounds");
    return;
  }
  this->GridSize[0] = sizeX;
  this->GridSize[1] = sizeY;
  this->GridSize[2] = sizeZ;

  this->IsVoxelGridValid = true;
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::RecomputeVoxelGrid()
{
  vtkNew<vtkPolyData> newOutput;
  newOutput->DeepCopy(this->Output);

  this->Clear();
  this->InitializeData();

  for (vtkIdType i = 0; i < newOutput->GetPoints()->GetNumberOfPoints(); i++)
  {
    this->AddPoint(newOutput, i);
  }
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::ExpandVoxelGrid(const double previousBounds[6],
  const uint64 previousGridSize[3])
{
  // Bounds difference
  uint64 diff[6];
  for (int i = 0; i < 6; i++)
  {
    diff[i] = static_cast<uint64>(std::ceil(std::abs(this->Bounds[i] - previousBounds[i])) /
      static_cast<uint64>(this->LeafSize));
  }

  // Update new bounds from previous bounds an a multiple of the leaf size to keep the previous
  // voxels
  for (int i = 0; i < 6; i++)
  {
    if (i % 2 == 0)
    {
      this->Bounds[i] = previousBounds[i] - static_cast<double>(diff[i]) * this->LeafSize;
    }
    else
    {
      this->Bounds[i] = previousBounds[i] + static_cast<double>(diff[i]) * this->LeafSize;
    }
  }

  // The bounds have changed, the voxel size must be updated
  this->UpdateVoxelSize();

  std::unordered_map<uint64, vtkIdType> newVoxelGrid;

  // Origin of the previous voxel grid in the new voxel grid
  Eigen::Array3i originId(diff[0], diff[2], diff[4]);

  // Copy the previous voxel grid in the new voxel grid
  for (auto it = this->VoxelGrid.begin(); it != this->VoxelGrid.end(); ++it)
  {
    int x = it->first % previousGridSize[0];
    int y = (it->first / previousGridSize[0]) % previousGridSize[1];
    int z = it->first / (previousGridSize[0] * previousGridSize[1]);
    Eigen::Array3i previousVoxelId = Eigen::Array3i(x, y, z);
    Eigen::Array3i newVoxelId = previousVoxelId + originId;
    uint64 newVoxelId1d = this->To1d(newVoxelId);
    newVoxelGrid[newVoxelId1d] = it->second;
  }

  // Update the voxel grid
  this->VoxelGrid = newVoxelGrid;
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::InitializeData()
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> cells;
  points->SetDataTypeToDouble();
  points->Resize(this->InitialNumberOfPoints);
  cells->SetNumberOfCells(this->InitialNumberOfPoints);
  this->Output->SetPoints(points);
  this->Output->SetVerts(cells);

  this->VoxelCount = 0;
  this->CurrentDataSize = this->InitialNumberOfPoints;
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::SetInitialNumberOfPoints(int nbPoints)
{
  this->InitialNumberOfPoints = nbPoints;
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::SetBounds(double xmin,
  double xmax,
  double ymin,
  double ymax,
  double zmin,
  double zmax)
{
  double bounds[6] = { xmin, xmax, ymin, ymax, zmin, zmax };

  double previousBounds[6];
  std::copy(this->Bounds, this->Bounds + 6, previousBounds);
  uint64 previousGridSize[3] = { this->GridSize[0], this->GridSize[1], this->GridSize[2] };
  // Update the bounds
  std::copy(bounds, bounds + 6, this->Bounds);
  this->UpdateVoxelSize();

  // Recompute the voxel grid if necessary
  if (this->VoxelGrid.empty())
  {
    return;
  }

  if (previousBounds[0] >= this->Bounds[0] && previousBounds[1] <= this->Bounds[1] &&
    previousBounds[2] >= this->Bounds[2] && previousBounds[3] <= this->Bounds[3] &&
    previousBounds[4] >= this->Bounds[4] && previousBounds[5] <= this->Bounds[5])
  {
    // The new bounds are a subset of the previous bounds
    // The previous voxel grid can be expanded
    this->ExpandVoxelGrid(previousBounds, previousGridSize);
  }
  else
  {
    // The new bounds are not a subset of the previous bounds
    // The voxel grid must be recomputed
    this->RecomputeVoxelGrid();
  }
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::SetLeafSize(double ls)
{
  if (ls <= 1e-6)
  {
    vtkErrorMacro("vtkVoxelGridProcessor::SetLeafSize : increase the leaf size, should be > 1e-6");
    return;
  }
  this->LeafSize = ls;
  this->UpdateVoxelSize();
  this->RecomputeVoxelGrid();
}

//------------------------------------------------------------------------------
bool vtkVoxelGridProcessor::ResizeData()
{
  if (this->VoxelCount > this->CurrentDataSize)
  {
    // Resize the data
    if (!this->Output->GetPoints()->Resize(this->CurrentDataSize + this->ResizeNumberOfPoints))
    {
      vtkErrorMacro("vtkVoxelGridProcessor::ResizeData : failed to resize the data");
      return false;
    }
    this->Output->GetVerts()->SetNumberOfCells(this->CurrentDataSize + this->ResizeNumberOfPoints);
    this->CurrentDataSize += this->ResizeNumberOfPoints;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::FreeUnusedMemory()
{
  if (this->VoxelPointDataInitialized)
  {
    this->Output->GetPoints()->Resize(this->VoxelCount);
    this->Output->GetVerts()->Squeeze();
    this->Output->GetPointData()->Squeeze();
  }
}

//------------------------------------------------------------------------------
void vtkVoxelGridProcessor::Clear()
{
  this->VoxelPointDataInitialized = false;
  this->Output->Initialize();
  this->VoxelGrid.clear();
  this->NumberOfPointsPerVoxel.clear();
}

//------------------------------------------------------------------------------
uint64 vtkVoxelGridProcessor::To1d(const Eigen::Array3i& voxelId3d) const
{
  uint64 ret = static_cast<uint64>(voxelId3d.z()) * GridSize[0] * GridSize[1] +
    static_cast<uint64>(voxelId3d.y()) * GridSize[0] + static_cast<uint64>(voxelId3d.x());
  return ret;
}

//------------------------------------------------------------------------------
Eigen::Array3i vtkVoxelGridProcessor::To3d(uint64 voxelId1d) const
{
  int x = voxelId1d % this->GridSize[0];
  int y = (voxelId1d / this->GridSize[0]) % this->GridSize[1];
  int z = voxelId1d / (this->GridSize[0] * this->GridSize[1]);
  return { x, y, z };
}

//------------------------------------------------------------------------------
uint64 vtkVoxelGridProcessor::PositionToVoxelId(const Eigen::Vector3d& position) const
{
  if (this->LeafSize <= 1e-6)
  {
    vtkErrorMacro(
      "vtkVoxelGridProcessor::PositionToVoxelId : increase leaf size, should be > 1e-6");
    return 0;
  }
  Eigen::Vector3d minBounds = { this->Bounds[0], this->Bounds[2], this->Bounds[4] };
  Eigen::Vector3d positionCount = (position - minBounds) / this->LeafSize;
  positionCount = positionCount.unaryExpr([](double p) { return std::floor(p); });
  return this->To1d(positionCount.array().round().template cast<int>());
}

//------------------------------------------------------------------------------
Eigen::Vector3d vtkVoxelGridProcessor::VoxelIdToPosition(uint64 voxelId) const
{
  Eigen::Vector3d minBounds = { this->Bounds[0], this->Bounds[2], this->Bounds[4] };
  Eigen::Vector3d voxelCenter = this->To3d(voxelId).cast<double>();
  voxelCenter *= this->LeafSize;
  voxelCenter += minBounds + Eigen::Vector3d::Constant(this->LeafSize) / 2.0;
  return voxelCenter;
}

//------------------------------------------------------------------------------
bool vtkVoxelGridProcessor::IsInBounds(const Eigen::Vector3d& position) const
{
  return (position.x() >= this->Bounds[0] && position.x() <= this->Bounds[1] &&
    position.y() >= this->Bounds[2] && position.y() <= this->Bounds[3] &&
    position.z() >= this->Bounds[4] && position.z() <= this->Bounds[5]);
}
