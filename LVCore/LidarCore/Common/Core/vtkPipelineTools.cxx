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
