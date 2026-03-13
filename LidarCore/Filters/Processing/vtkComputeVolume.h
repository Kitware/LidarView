/*=========================================================================

  Program: LidarView
  Module:  vtkComputeVolume.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_COMPUTE_VOLUME_H
#define VTK_COMPUTE_VOLUME_H

// VTK
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersProcessingModule.h"

#include <memory> // for std::unique_ptr

/**
 * @brief vtkComputeVolume computes the volume of space formed by an input plane
 *        and a point cloud
 *        Inputs:
 *        - a 3D pointcloud
 *        - a plane defined by a plane normal and a plane origin
 *        Outputs:
 *        - rasterized pointcloud
 *        - estimated volume
 */

class LVFILTERSPROCESSING_EXPORT vtkComputeVolume : public vtkPolyDataAlgorithm
{
public:
  static vtkComputeVolume* New();
  vtkTypeMacro(vtkComputeVolume, vtkPolyDataAlgorithm)

  /**
   * Set the size of the leaf used to rasterize the pointcloud.
   * It should be equivalent to or greater than the average resolution of the input pointcloud.
   */
  void SetGridResolution(double resolution);

  /**
   * Set to enable the interpolation between empty bin when scan resolution is less than the grid
   * resolution InterpolationThreshold indique the max consecutive empty bin allowed for the
   * interpolation
   */
  vtkSetMacro(EnableInterpolation, bool);
  vtkSetMacro(InterpolationThreshold, int);

protected:
  vtkComputeVolume();
  ~vtkComputeVolume() = default;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkComputeVolume(const vtkComputeVolume&) = delete;
  void operator=(const vtkComputeVolume&) = delete;

  bool EnableInterpolation = false;
  int InterpolationThreshold = 4;

  /**
   * Internals parameters and functions to compute volume
   */
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif // VTK_COMPUTE_VOLUME_H
