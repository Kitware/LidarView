/*=========================================================================

  Program: LidarView
  Module:  vtkComputeNormalAdaptive.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_COMPUTE_NORMAL_ADAPTIVE_H
#define VTK_COMPUTE_NORMAL_ADAPTIVE_H

// VTK
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersProcessingModule.h"

class LVFILTERSPROCESSING_EXPORT vtkComputeNormalAdaptive : public vtkPolyDataAlgorithm
{
public:
  static vtkComputeNormalAdaptive* New();
  vtkTypeMacro(vtkComputeNormalAdaptive, vtkPolyDataAlgorithm)

  ///@{
  /**
   *  Set search mode: 0 for knn and 1 for radius
   *  Set number of neighbours threshold
   *  Set neighbours radius threshold
   *  Set estimated noise of point cloud
   *  Set whether to denoise point cloud using computed normals
   *  Set whether to orient normal to the exterior of the edge
   */
  vtkSetMacro(SearchMode, int);
  vtkSetMacro(NumberNeighbors, int);
  vtkSetMacro(Radius, float);
  vtkSetMacro(Noise, float);
  vtkSetMacro(Denoise, bool);
  vtkSetMacro(Orient, bool);

protected:
  vtkComputeNormalAdaptive() = default;
  ~vtkComputeNormalAdaptive() = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkComputeNormalAdaptive(const vtkComputeNormalAdaptive&) = delete;
  void operator=(const vtkComputeNormalAdaptive&) = delete;

  int SearchMode = 0;
  int NumberNeighbors = 3;
  float Radius = 0.1f;
  float Noise = 1e-6f;
  bool Denoise = false;
  bool Orient = false;
};

#endif // VTK_COMPUTE_NORMAL_ADAPTIVE_H
