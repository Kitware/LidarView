/*=========================================================================

  Program: LidarView
  Module:  TestComputeVolume.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkComputeVolume.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <Eigen/Dense>
#include <cmath>

int TestComputeVolume(int, char*[])
{
  // The volume to estimate is a 4 * 2 * 2 rectangle
  double rectL = 4.0;
  double rectW = 2.0;
  double rectH = 2.0;
  double rectVolume = rectL * rectW * rectH;
  // Create an input pointcloud of the above rectangle for compute volume filter
  double coordZ = rectH;
  double resolution = 0.01;
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  // Create a pointcloud which forms a 4 * 2 * 2 rectangle
  for (double coordX = 0.0; coordX < rectL + 1e-6; coordX = coordX + resolution)
  {
    for (double coordY = 0.0; coordY < rectW + 1e-6; coordY = coordY + resolution)
    {
      double pt[3] = { coordX, coordY, coordZ };
      points->InsertNextPoint(pt);
    }
  }
  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);

  // Create input plane for compute volume filter
  vtkSmartPointer<vtkPolyData> planeSource = vtkSmartPointer<vtkPolyData>::New();
  // Set plane points
  vtkSmartPointer<vtkPoints> planePoints = vtkSmartPointer<vtkPoints>::New();
  planePoints->SetDataTypeToDouble();
  double bottomLeftPt[3] = { -1e-6, -1e-6, 0.0 };
  double bottomRightPt[3] = { rectL + resolution, -1e-6, 0.0 };
  double topLeftPt[3] = { -1e-6, rectW + resolution, 0.0 };
  double topRightPt[3] = { rectL + resolution, rectW + resolution, 0.0 };
  planePoints->InsertNextPoint(bottomLeftPt);
  planePoints->InsertNextPoint(bottomRightPt);
  planePoints->InsertNextPoint(topLeftPt);
  planePoints->InsertNextPoint(topRightPt);
  planeSource->SetPoints(planePoints);
  // Set plane normal
  vtkNew<vtkDoubleArray> normalArray;
  normalArray->SetName("Normals");
  normalArray->SetNumberOfComponents(3);
  normalArray->SetNumberOfTuples(1);
  double normal[3] = { 0, 0, 1 };
  normalArray->SetTuple(0, normal);
  planeSource->GetPointData()->AddArray(normalArray);

  // apply compute volume filter
  auto filter = vtkSmartPointer<vtkComputeVolume>::New();
  filter->SetInputData(0, polyData);
  filter->SetInputData(1, planeSource);
  filter->SetGridResolution(resolution);
  filter->Update();

  // Get the estimated volume value
  vtkFieldData* fieldData = filter->GetOutput()->GetFieldData();
  if (!fieldData)
  {
    return EXIT_FAILURE;
  }
  vtkDoubleArray* array = vtkDoubleArray::SafeDownCast(fieldData->GetAbstractArray("Volume"));
  if (!array || array->GetNumberOfTuples() != 1)
  {
    return EXIT_FAILURE;
  }
  double estimatedVolume = -1.0;
  array->GetTuple(0, &estimatedVolume);
  // Check the estimated volume
  if (estimatedVolume < 1e-6 || std::fabs(estimatedVolume - rectVolume) > 0.01 * rectVolume)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
