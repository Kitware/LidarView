/*=========================================================================

  Program:   LidarView
  Module:    TestVtkCustomTransformInterpolator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// VTK
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkTesting.h>

#include <iostream>

// LOCAL
#include "vtkCustomTransformInterpolator.h"
#include "vtkTemporalTransforms.h"
#include "vtkTemporalTransformsReader.h"

#define VTK_SUCCESS 0
#define VTK_FAILURE 1

//-----------------------------------------------------------------------------
int TestVtkCustomTransformInterpolator(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string dataRoot = testing->GetDataRoot();
  std::string continuousTrajectory = dataRoot + "/Slam/trajectory.poses";

  vtkNew<vtkTemporalTransformsReader> reader;
  reader->SetFileName(continuousTrajectory.c_str());
  reader->Update();

  vtkSmartPointer<vtkPolyData> polyData = vtkPolyData::SafeDownCast(reader->GetOutput());

  // Create the temporal transform
  auto temporalTransform = vtkTemporalTransforms::CreateFromPolyData(polyData);
  vtkSmartPointer<vtkCustomTransformInterpolator> interpolator =
    temporalTransform->CreateInterpolator();
  interpolator->SetInterpolationType(vtkCustomTransformInterpolator::INTERPOLATION_TYPE_LINEAR);

  // Test of the continuity of the trajectory to within an offset
  if (!interpolator->IsTimeContinuous(0.5))
  {
    std::cout << "Error: " << continuousTrajectory
              << " should be continuous with a 0.5s offset but is not. " << std::endl;
    return VTK_FAILURE;
  }

  if (interpolator->IsTimeContinuous(0.03))
  {
    std::cout << "Error: " << continuousTrajectory
              << " should not be continuous with a 0.03s offset but is . " << std::endl;
    return VTK_FAILURE;
  }

  // Test of the range of the trajectory to within an offset
  if (!interpolator->IsTimeInRange(1455.8, 0.5))
  {
    std::cout << "Error: 1455.8 with 0.5s offset should be in range but is not. " << std::endl;
    return VTK_FAILURE;
  }

  if (interpolator->IsTimeInRange(1455.3, 0.5))
  {
    std::cout << "Error: 1455.3 with 0.5s offset should not be in range but is. " << std::endl;
    return VTK_FAILURE;
  }

  // Test of the validity of the trajectory to within an offset
  for (int i = 0.; i < 50; i++)
  {
    // time change between 1456 and 1462
    double time = 1456. + (1462 - 1456) * i / 50.;
    if (!interpolator->IsTimeValid(time, 0.5))
    {
      std::cout << "Error: " << time << " should be valid but is not. " << std::endl;
      return VTK_FAILURE;
    }
  }

  double time = 1456.54;

  if (!interpolator->IsTimeValid(time, 0.03))
  {
    std::cout << "Error: " << time << " should be valid with a 0.03s but is not. " << std::endl;
    return VTK_FAILURE;
  }

  if (interpolator->IsTimeValid(time, 0.02))
  {
    std::cout << "Error: " << time << " should not be valid with a 0.02s but is not. " << std::endl;
    return VTK_FAILURE;
  }

  return VTK_SUCCESS;
}
