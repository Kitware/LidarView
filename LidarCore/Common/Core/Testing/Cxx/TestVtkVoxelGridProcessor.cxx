/*=========================================================================

  Program:   LidarView
  Module:    TestVtkVoxelGridProcessor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// STD
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

// VTK
#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

// LOCAL
#include "vtkVoxelGridProcessor.h"

#define VTK_SUCCESS 0
#define VTK_FAILURE 1

#define e 0.0000001

//-----------------------------------------------------------------------------
int TestVtkVoxelGridProcessor(int, char*[])
{
  vtkNew<vtkPolyData> polyData;
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(-0.5, -0.5, -0.5);
  points->InsertNextPoint(0.5, -0.5, -0.5);
  points->InsertNextPoint(-0.5, 0.5, -0.5);
  points->InsertNextPoint(0.5, 0.5, -0.5);
  points->InsertNextPoint(-0.5, -0.5, 0.5);
  points->InsertNextPoint(0.5, -0.5, 0.5);
  points->InsertNextPoint(-0.5, 0.5, 0.5);
  points->InsertNextPoint(0.5, 0.5, 0.5);
  points->InsertNextPoint(0.2, 0.2, 0.2);
  points->InsertNextPoint(12.0, 0.2, 0.2);
  polyData->SetPoints(points);

  // Create a voxel grid with 8 voxels, 2 in each direction betweexpectedKeysen -1 and 1.
  // So the voxel ids go from 0 to 7.
  vtkNew<vtkVoxelGridProcessor> voxelGrid;
  voxelGrid->SetBounds(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  voxelGrid->SetLeafSize(1.0);
  // Test FIRST sampling mode
  voxelGrid->SetSampling(vtkVoxelGridProcessor::FIRST);

  // One point per voxel is added
  for (int i = 0; i < 8; ++i)
  {
    voxelGrid->AddPoint(polyData, i);
  }

  const std::unordered_map<uint64, vtkIdType>& voxels = voxelGrid->GetVoxelGrid();
  // The voxel grid should contain 8 voxels
  if (voxels.size() != 8)
  {
    std::cout << "Error: wrong voxel grid size" << std::endl;
    return VTK_FAILURE;
  }

  // The voxel ids should go from 0 to 7 and should be all filled
  for (auto& it : voxels)
  {
    if (static_cast<int>(it.first) != it.second)
    {
      std::cout << "Error: voxel id " << it.first << " should be " << it.second
                << ", wrong voxel filled" << std::endl;
      return VTK_FAILURE;
    }
  }

  // Add a new point in an already filled voxel (voxel id = 7)
  voxelGrid->AddPoint(polyData, 8);
  vtkIdType id = voxels.at(7);
  if (id != 7)
  {
    std::cout
      << "Error: wrong voxel id : FIRST sampling mode, first point added in a voxel should be kept"
      << std::endl;
    return VTK_FAILURE;
  }
  // vtkVoxelGridProcessor::FIRST sampling mode is used, the first point added in a voxel should be
  // kept
  double point[3];
  voxelGrid->GetOutput()->GetPoint(id, point);
  for (int i = 0; i < 3; ++i)
  {
    if (std::abs(point[i] - 0.5) > e)
    {
      std::cout << "Error: wrong point coordinates : FIRST sampling mode, first point added in a "
                   "voxel should be kept"
                << std::endl;
      return VTK_FAILURE;
    }
  }

  // Test LAST sampling mode
  voxelGrid->SetSampling(vtkVoxelGridProcessor::LAST);
  voxelGrid->AddPoint(polyData, 8);
  // vtkVoxelGridProcessor::LAST sampling mode is used, the last point added in a voxel should be
  // kept
  voxelGrid->GetOutput()->GetPoint(id, point);
  for (int i = 0; i < 3; ++i)
  {
    if (std::abs(point[i] - 0.2) > e)
    {
      std::cout << "Error: wrong point coordinates : LAST sampling mode, last point added in a "
                   "voxel should be kept"
                << std::endl;
      return VTK_FAILURE;
    }
  }

  // Add point out of bounds : should not be added
  voxelGrid->AddPoint(polyData, 9);
  if (voxels.size() != 8)
  {
    std::cout
      << "Error: wrong voxel id : LAST sampling mode, last point added in a voxel should be kept"
      << std::endl;
    return VTK_FAILURE;
  }

  // Test ExpandVoxelGrid method when the previous bounds are contained in the new bounds
  voxelGrid->SetBounds(-1.5, 1.0, -1.5, 1.5, -1.5, 1.5);

  double* bounds = voxelGrid->GetBounds();
  // The new bounds should be the sum of the previous bounds multiplied by a multiple of LeafSize
  double expectedBounds[6] = { -2.0, 1.0, -2.0, 2.0, -2.0, 2.0 };
  for (int i = 0; i < 6; ++i)
  {
    if (bounds[i] != expectedBounds[i])
    {
      std::cout << "Error: wrong bounds : ExpandVoxelGrid (method used by SetBounds) failed"
                << std::endl;
      return VTK_FAILURE;
    }
  }
  std::vector<uint64> expectedKeys = { 16, 32, 17, 19, 20, 28, 29, 31 };
  int j = 0;
  for (auto& it : voxels)
  {
    if (std::find(expectedKeys.begin(), expectedKeys.end(), it.first) == expectedKeys.end())
    {
      std::cout << "Error: wrong voxel id : ExpandVoxelGrid (method used by SetBounds) failed"
                << std::endl;
      return VTK_FAILURE;
    }
    j++;
  }

  // New bounds are smaller than the previous bounds, the voxel grid should be recomputed
  voxelGrid->SetBounds(0., 1.8, 0., 1.8, .0, 1.8);
  // A single point is in bounds
  if (voxels.size() != 1)
  {
    std::cout << "Error: wrong voxel size : RecomputeBounds (method used by SetBounds) failed"
              << std::endl;
    return VTK_FAILURE;
  }
  id = voxels.at(0);
  voxelGrid->GetOutput()->GetPoint(id, point);
  for (int i = 0; i < 3; ++i)
  {
    if (std::abs(point[i] - 0.2) > e)
    {
      std::cout << "Error: wrong point coordinates : RecomputeBounds (method used by SetBounds) "
                   "failed"
                << std::endl;
      return VTK_FAILURE;
    }
  }

  // Clear the voxel grid
  voxelGrid->Clear();
  // The voxel grid unordered map should be empty and the output polydata should not contain
  // vtkPoints
  if (!voxels.empty() || voxelGrid->GetOutput()->GetPoints())
  {
    std::cout << "Error: Clear method failed" << std::endl;
    return VTK_FAILURE;
  }

  // Test CENTER_POINT sampling mode
  voxelGrid->SetSampling(vtkVoxelGridProcessor::CENTER_POINT);
  // Single voxel voxel grid
  voxelGrid->SetBounds(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
  // Add 0.2, 0.2, 0.2 point
  voxelGrid->AddPoint(polyData, 8);
  id = voxels.at(0);
  voxelGrid->GetOutput()->GetPoint(id, point);
  // The center of the voxel should be 0.5, 0.5, 0.5
  for (int i = 0; i < 3; ++i)
  {
    if (std::abs(point[i] - 0.5) > e)
    {
      std::cout << "Error: wrong point coordinates : CENTER_POINT sampling mode failed"
                << std::endl;
      return VTK_FAILURE;
    }
  }

  vtkNew<vtkPoints> points1;
  points1->InsertNextPoint(0.2, 0.1, 0.1);
  points1->InsertNextPoint(0.2, 0.3, 0.1);
  points1->InsertNextPoint(0.2, 0.2, 0.25);
  polyData->SetPoints(points1);

  voxelGrid->Clear();
  // Test CENTROID sampling mode
  voxelGrid->SetSampling(vtkVoxelGridProcessor::CENTROID);

  for (int i = 0; i < 3; ++i)
  {
    voxelGrid->AddPoint(polyData, i);
  }
  id = voxels.at(0);
  voxelGrid->GetOutput()->GetPoint(id, point);
  // The output point should be the centroid of the 3 points : 0.2, 0.2, 0.15
  if (std::abs(point[0] - 0.2) > e || std::abs(point[1] - 0.2) > e || std::abs(point[2] - 0.15) > e)
  {
    std::cout << "Error: wrong point coordinates : CENTROID sampling mode failed" << std::endl;
    return VTK_FAILURE;
  }

  // Add intensity to the points
  vtkNew<vtkDoubleArray> intensity;
  intensity->SetName("intensity");
  intensity->InsertNextValue(0.1);
  intensity->InsertNextValue(0.3);
  intensity->InsertNextValue(0.2);
  polyData->GetPointData()->AddArray(intensity);

  voxelGrid->Clear();
  // Test MAX_INTENSITY sampling mode
  voxelGrid->SetSampling(vtkVoxelGridProcessor::MAX_INTENSITY);
  for (int i = 0; i < 3; ++i)
  {
    voxelGrid->AddPoint(polyData, i);
  }

  id = voxels.at(0);
  voxelGrid->GetOutput()->GetPoint(id, point);
  // The output point should be the second point with the highest intensity : 0.2, 0.3, 0.1
  if (std::abs(point[0] - 0.2) > e || std::abs(point[1] - 0.3) > e || std::abs(point[2] - 0.1) > e)
  {
    std::cout << "Error: wrong point coordinates : MAX_INTENSITY sampling mode failed" << std::endl;
    return VTK_FAILURE;
  }

  return VTK_SUCCESS;
}
