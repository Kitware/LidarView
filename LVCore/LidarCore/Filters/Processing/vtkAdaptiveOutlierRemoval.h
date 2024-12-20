/*=========================================================================

  Program: LidarView
  Module:  vtkAdaptiveOutlierRemoval.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_ADAPTIVE_OUTLIER_REMOVAL_H
#define VTK_ADAPTIVE_OUTLIER_REMOVAL_H

// VTK
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include "lvFiltersProcessingModule.h"

/**
 * @brief This filter identifies outliers in a point cloud based on the average distance of
 neighboring points. The threshold can be defined adaptivly with respect to the depth of points.
 */

class LVFILTERSPROCESSING_EXPORT vtkAdaptiveOutlierRemoval : public vtkPolyDataAlgorithm
{
public:
  static vtkAdaptiveOutlierRemoval* New();
  vtkTypeMacro(vtkAdaptiveOutlierRemoval, vtkPolyDataAlgorithm)

protected:
  vtkAdaptiveOutlierRemoval();
  ~vtkAdaptiveOutlierRemoval() = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAdaptiveOutlierRemoval(const vtkAdaptiveOutlierRemoval&) = delete;
  void operator=(const vtkAdaptiveOutlierRemoval&) = delete;
};

#endif // VTK_ADAPTIVE_OUTLIER_REMOVAL_H
