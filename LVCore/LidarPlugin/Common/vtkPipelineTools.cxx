// LOCAL
#include "vtkPipelineTools.h"

// VTK
#include <vtkStreamingDemandDrivenPipeline.h>


std::vector<double> getTimeSteps(vtkInformation* info)
{
  const int steps = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  std::vector<double> timesteps(steps);
  for (int i = 0; i < steps; ++i)
  {
    timesteps[i] = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), i);
  }
  return timesteps;
}
