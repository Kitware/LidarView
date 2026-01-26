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

  ///@{
  /*
   * Step detection threshold (meters).
   */
  vtkSetMacro(ZChangeMin, double);
  vtkGetMacro(ZChangeMin, double);
  ///@}

  ///@{
  /*
   * Slice half-thickness along plane normal (meters).
   */
  vtkSetMacro(SliceHalfThickness, double);
  vtkGetMacro(SliceHalfThickness, double);
  ///@}

  ///@{
  /*
   * Non-negative lateral distances from trajectory centerline (meters).
   * Left and right correspond to opposite sides of the trajectory, so both
   * values must be >= 0.
   */
  vtkSetMacro(SliceLeftDistance, double);
  vtkGetMacro(SliceLeftDistance, double);
  vtkSetMacro(SliceRightDistance, double);
  vtkGetMacro(SliceRightDistance, double);
  ///@}

  ///@{
  /*
   * Vertical bounds in slice frame along v (meters).
   * These are signed coordinates on the same vertical axis, so both bounds
   * may be on the same side of the trajectory (both positive or both negative).
   */
  vtkSetMacro(SliceDownwardDistance, double);
  vtkGetMacro(SliceDownwardDistance, double);
  vtkSetMacro(SliceUpwardDistance, double);
  vtkGetMacro(SliceUpwardDistance, double);
  ///@}

protected:
  vtkCurbDetectorOffline();
  ~vtkCurbDetectorOffline() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCurbDetectorOffline(const vtkCurbDetectorOffline&) = delete;
  void operator=(const vtkCurbDetectorOffline&) = delete;

  // Minimum vertical step (in meters) on the smoothed profile
  // required to detect a curb candidate.
  double ZChangeMin = 0.1;

  // Half thickness of the slicing region measured
  // perpendicular to the trajectory (meters).
  double SliceHalfThickness = 0.2;

  // Maximum distance to the left/right of the trajectory to include in each slice (meters).
  // Values must be non-negative because left and right are opposite sides.
  // Negative values are treated as invalid.
  double SliceLeftDistance = 5.0;
  double SliceRightDistance = 5.0;

  // Lower/Upper vertical limits on the same vertical axis in slice frame (meters).
  // They are signed coordinates and can both be on the same side.
  double SliceDownwardDistance = -5.0;
  double SliceUpwardDistance = -2.0;
};

#endif // VTK_CURB_DETECTOR_OFFLINE_H
