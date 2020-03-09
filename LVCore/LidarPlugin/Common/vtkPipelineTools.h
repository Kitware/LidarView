// Collection of common tools related to pipeline management

#ifndef VTK_PIPELINE_TOOLS_H
#define VTK_PIPELINE_TOOLS_H

// VTK
#include <vtkInformation.h>

// STD
#include <vector>

/** @brief getTimeSteps get all timesteps from a vtkInformation
 *         Can be used to synchronize inputs with different timelines
 */
std::vector<double> getTimeSteps(vtkInformation* info);

# endif  // VTK_PIPELINE_TOOLS-H

