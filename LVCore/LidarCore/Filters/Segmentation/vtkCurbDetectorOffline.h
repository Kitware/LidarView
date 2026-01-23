/*=========================================================================

  Program:   LidarView
  Module:    vtkCurbDetectorOffline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_CURB_DETECTOR_OFFLINE_H
#define VTK_CURB_DETECTOR_OFFLINE_H

#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersSegmentationModule.h"

class vtkInformation;
class vtkInformationVector;
/**
 * vtkCurbDetectorOffline takes an aggregated point cloud and an associated
 * trajectory as inputs.
 *
 * For each consecutive trajectory segment, a local slicing plane is built
 * perpendicular to the trajectory direction. Points close to this plane are
 * selected and analyzed in a local slice frame.
 *
 * Within each slice, points are grouped laterally relative to the trajectory,
 * a vertical profile is constructed using median-based binning and light
 * smoothing, and significant vertical steps in this profile are detected as
 * curb candidates.
 */
class LVFILTERSSEGMENTATION_EXPORT vtkCurbDetectorOffline : public vtkPolyDataAlgorithm
{
public:
  static vtkCurbDetectorOffline* New();
  vtkTypeMacro(vtkCurbDetectorOffline, vtkPolyDataAlgorithm);

protected:
  vtkCurbDetectorOffline();
  ~vtkCurbDetectorOffline() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCurbDetectorOffline(const vtkCurbDetectorOffline&) = delete;
  void operator=(const vtkCurbDetectorOffline&) = delete;
};

#endif // VTK_CURB_DETECTOR_OFFLINE_H
