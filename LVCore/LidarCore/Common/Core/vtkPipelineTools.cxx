/*=========================================================================

  Program:   LidarView
  Module:    vtkPipelineTools.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// LOCAL
#include "vtkPipelineTools.h"

// VTK
#include <vtkStreamingDemandDrivenPipeline.h>

std::vector<double> getTimeSteps(vtkInformation* info)
{
  if (!info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    return std::vector<double>();
  }
  const int steps = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  std::vector<double> timesteps(steps);
  for (int i = 0; i < steps; ++i)
  {
    timesteps[i] = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), i);
  }
  return timesteps;
}
