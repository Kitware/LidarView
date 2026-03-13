/*=========================================================================

  Program:   LidarView
  Module:    vtkPipelineTools.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Collection of common tools related to pipeline management

#ifndef VTK_PIPELINE_TOOLS_H
#define VTK_PIPELINE_TOOLS_H

// VTK
#include <vtkInformation.h>

// STD
#include <vector>

#include "lvCommonCoreModule.h"

/** @brief getTimeSteps get all timesteps from a vtkInformation
 *         Can be used to synchronize inputs with different timelines
 */
std::vector<double> LVCOMMONCORE_EXPORT getTimeSteps(vtkInformation* info);

#endif // VTK_PIPELINE_TOOLS-H
