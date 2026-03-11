/*=========================================================================

  Program:   LidarView
  Module:    TestVtkMergePointsToPolyDataHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// STD
#include <iostream>
#include <unordered_map>

// VTK
#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

// LOCAL
#include "vtkMergePointsToPolyDataHelper.h"

#define VTK_SUCCESS 0
#define VTK_FAILURE 1

#define e 0.0000001

//-----------------------------------------------------------------------------
int TestVtkMergePointsToPolyDataHelper(int, char*[])
{
  vtkNew<vtkPolyData> polyData;
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(50.0, 3.0, 5.0);
  points->InsertNextPoint(0.5, -12.5, -0.5);
  points->InsertNextPoint(-0.5, 0.5, -0.5);
  polyData->SetPoints(points);
  // Add intensity to the points
  vtkNew<vtkDoubleArray> intensity;
  intensity->SetName("intensity");
  intensity->InsertNextValue(0.1);
  intensity->InsertNextValue(0.3);
  intensity->InsertNextValue(0.2);
  polyData->GetPointData()->AddArray(intensity);

  // Create mergePointsToPolyDataHelper
  vtkNew<vtkMergePointsToPolyDataHelper> mergePointsToPolyDataHelper;

  // Add points to mergePointsToPolyDataHelper
  for (int i = 0; i < 3; ++i)
  {
    mergePointsToPolyDataHelper->AddPoint(polyData, i);
  }

  if (mergePointsToPolyDataHelper->GetOutput()->GetPoints()->GetNumberOfPoints() != 3)
  {
    std::cout << "Error: wrong number of points added in the output. " << std::endl;
    return VTK_FAILURE;
  }

  double point[3];
  int pointId = 0;
  mergePointsToPolyDataHelper->GetOutput()->GetPoint(pointId, point);
  // The output point should be the second point with the highest intensity : 0.2, 0.3, 0.1
  if (std::abs(point[0] - 50.0) > e || std::abs(point[1] - 3.0) > e || std::abs(point[2] - 5.0) > e)
  {
    std::cout << "Error: wrong point coordinates for point id=" << pointId << std::endl;
    return VTK_FAILURE;
  }
  pointId = 2;
  mergePointsToPolyDataHelper->GetOutput()->GetPoint(pointId, point);
  // The output point should be the second point with the highest intensity : 0.2, 0.3, 0.1
  if (std::abs(point[0] - -0.5) > e || std::abs(point[1] - 0.5) > e ||
    std::abs(point[2] - -0.5) > e)
  {
    std::cout << "Error: wrong point coordinates for point id=" << pointId << std::endl;
    return VTK_FAILURE;
  }

  // The intensity of the output points should be the same as the input points
  double expectedIntensity[3] = { 0.1, 0.3, 0.2 };
  for (int i = 0; i < 3; ++i)
  {
    double intensityValue =
      mergePointsToPolyDataHelper->GetOutput()->GetPointData()->GetArray("intensity")->GetTuple1(i);
    if (std::abs(intensityValue - expectedIntensity[i]) > e)
    {
      std::cout << "Error: wrong intensity value for point id=" << i << std::endl;
      return VTK_FAILURE;
    }
  }

  // Test Add point with custom coordinates and the data of the second point of the input polydata
  double newPoint[3] = { 3.0, 3.0, 3.0 };
  pointId = 3; // The pointId should be 3 because the output polydata already has 3 points
  mergePointsToPolyDataHelper->AddPoint(polyData, 1, newPoint);
  mergePointsToPolyDataHelper->GetOutput()->GetPoint(pointId, point);
  if (std::abs(point[0] - 3.0) > e || std::abs(point[1] - 3.0) > e || std::abs(point[2] - 3.0) > e)
  {
    std::cout << "Error: wrong point coordinates for point id=" << pointId << std::endl;
    return VTK_FAILURE;
  }
  // The intensity of the new point should be the same as the second point of the input polydata
  double intensityValue = mergePointsToPolyDataHelper->GetOutput()
                            ->GetPointData()
                            ->GetArray("intensity")
                            ->GetTuple1(pointId);
  if (std::abs(intensityValue - 0.3) > e)
  {
    std::cout << "Error: wrong intensity value for the new point added, id=" << pointId
              << std::endl;
    return VTK_FAILURE;
  }

  // Create a second polyData
  vtkNew<vtkPolyData> polyData1;
  vtkNew<vtkPoints> points1;
  points1->InsertNextPoint(50.0, 3.0, 5.0);
  polyData1->SetPoints(points1);
  // Add intensity to the points
  vtkNew<vtkDoubleArray> intensity1;
  intensity1->SetName("intensity");
  intensity1->InsertNextValue(10.0);
  polyData1->GetPointData()->AddArray(intensity1);

  // Add a point from another polydata
  pointId = 4; // The pointId should be 4 because the output polydata already has 4 points
  mergePointsToPolyDataHelper->AddPoint(polyData1, 0);
  if (mergePointsToPolyDataHelper->GetOutput()->GetPoints()->GetNumberOfPoints() != 5)
  {
    std::cout << "Error: wrong number of points added from different polydata" << std::endl;
    return VTK_FAILURE;
  }
  mergePointsToPolyDataHelper->GetOutput()->GetPoint(pointId, point);
  if (std::abs(point[0] - 50.0) > e || std::abs(point[1] - 3.0) > e || std::abs(point[2] - 5.0) > e)
  {
    std::cout << "Error: wrong point coordinates from a different polydata for point id=" << pointId
              << std::endl;
    return VTK_FAILURE;
  }
  // The intensity of the new point should be the same as the second point of the input polydata
  intensityValue = mergePointsToPolyDataHelper->GetOutput()
                     ->GetPointData()
                     ->GetArray("intensity")
                     ->GetTuple1(pointId);
  if (std::abs(intensityValue - 10.0) > e)
  {
    std::cout
      << "Error: wrong intensity value for the new point added from a different polydata, id="
      << pointId << std::endl;
    return VTK_FAILURE;
  }

  // Test Clear
  mergePointsToPolyDataHelper->Clear();
  if (mergePointsToPolyDataHelper->GetOutput()->GetPoints())
  {
    std::cout << "Error: output polydata should be empty after Clear" << std::endl;
    return VTK_FAILURE;
  }

  return VTK_SUCCESS;
}
