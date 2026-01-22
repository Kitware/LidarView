/*=========================================================================

  Program:   LidarView
  Module:    TestTrailingFrame.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <iostream>

#include "vtkSelectPointsByScalars.h"

namespace
{
}
// Create a simple point cloud with 3 points
vtkSmartPointer<vtkPolyData> CreatePointCloud()
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);

  vtkNew<vtkPolyData> pd;
  pd->SetPoints(points.GetPointer());
  return pd;
}

int TestSelectPointsByScalars(int, char*[])
{
  vtkSmartPointer<vtkPolyData> dataset = CreatePointCloud();

  // Create a scalar array and attach it to the point data
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("TestScalars");

  scalars->InsertNextValue(0.5); // point 0 → scalar 0.5
  scalars->InsertNextValue(1.0); // point 1 → scalar 1.0
  scalars->InsertNextValue(2.0); // point 2 → scalar 2.0

  dataset->GetPointData()->SetScalars(scalars);

  // Use the custom filter
  vtkNew<vtkSelectPointsByScalars> selector;
  selector->SetInputData(dataset);

  // Specify which scalar array to use
  selector->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "TestScalars");
  selector->SetNumberOfValues(1);

  // Select points whose scalar value == 1.0
  selector->SetValue(0, 1.0);
  selector->Update();

  // Get the output polydata (should contain only 1 point: (1,0,0) with scalar=1.0)
  vtkPolyData* output = vtkPolyData::SafeDownCast(selector->GetOutput());

  if (!output)
  {
    std::cerr << "Error: output is not a vtkPolyData." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetNumberOfPoints() != 1)
  {
    std::cerr << "Error: output has wrong number of points." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetPointData()->GetScalars()->GetNumberOfTuples() != 1)
  {
    std::cerr << "Error: output has wrong number of scalars." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetPointData()->GetScalars()->GetComponent(0, 0) != 1.0)
  {
    std::cerr << "Error: output has wrong scalar value." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
