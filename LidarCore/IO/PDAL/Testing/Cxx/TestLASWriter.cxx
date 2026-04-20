/*=========================================================================

  Program:   LidarView
  Module:    TestLASWritter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

#include <iostream>

#include "vtkLASWriter.h"

int TestLASWriter(int argc, char* argv[])
{
  // Create a simple polydata with 4 points
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(2.0, 0.0, 0.0);
  points->InsertNextPoint(0.0, 2.0, 0.0);
  points->InsertNextPoint(0.0, 0.0, 2.0);

  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);

  vtkNew<vtkLASWriter> writer;
  writer->SetInputData(polydata);

  // Get temporary filename
  const char* tempFile =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "TMPDIR", "TestvtkLASWriter.las");
  writer->SetFileName(tempFile);

  // Activate auto offset and check that the computed center is correct
  // The center of mass of the 4 points is (0.5, 0.5, 0.5)
  writer->SetAutoOffset(true);

  writer->Write();

  // Check that the computed offset is correct
  double* offset = writer->GetOffset();
  if (fabs(offset[0] - 0.5) > 1e-6 || fabs(offset[1] - 0.5) > 1e-6 || fabs(offset[2] - 0.5) > 1e-6)
  {
    std::cerr << "AutoOffset computed center is incorrect: " << offset[0] << ", " << offset[1]
              << ", " << offset[2] << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
