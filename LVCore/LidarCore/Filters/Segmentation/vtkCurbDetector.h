/*=========================================================================

  Program:   LidarView
  Module:    vtkCurbDetector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_CURB_DETECTOR_H
#define VTK_CURB_DETECTOR_H

#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersSegmentationModule.h"

/**
 * vtkCurbDetector extracts curb points in real-time from a LiDAR point cloud. It focuses on
 * a configurable Z-slice and 3D region of interest to detect the left
 * and right curb edges.
 *
 * Internally, the filter orders points per lidar ring, flags step edges
 * using Z-height differences, splits candidates by the sign of X into left/right,
 * and applies 3D line RANSAC on each side to retain inliers while rejecting
 * outliers. It assumes X roughly corresponds to the lateral direction.
 */
class vtkInformation;
class vtkInformationVector;
class LVFILTERSSEGMENTATION_EXPORT vtkCurbDetector : public vtkPolyDataAlgorithm
{
public:
  static vtkCurbDetector* New();
  vtkTypeMacro(vtkCurbDetector, vtkPolyDataAlgorithm);

  // ZValue: keep all points with z == ZValue
  vtkSetMacro(ZValue, double);
  vtkGetMacro(ZValue, double);

  // Tolerance for equality check: keep if |z - ZValue| <= Tolerance
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

  ///@{
  /**
   * Step detection thresholds
   */
  vtkSetMacro(ZChangeMin, double);
  vtkGetMacro(ZChangeMin, double);
  ///@}

  ///@{
  /**
   * Region-of-interest box defined by its position and size.
   */
  vtkSetVector3Macro(BoxPosition, double);
  vtkGetVector3Macro(BoxPosition, double);
  void SetBoxScale(double sx, double sy, double sz);
  void SetBoxScale(const double s[3]);
  vtkGetVector3Macro(BoxScale, double);
  ///@}

protected:
  vtkCurbDetector();
  ~vtkCurbDetector() override = default;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCurbDetector(const vtkCurbDetector&) = delete;
  void operator=(const vtkCurbDetector&) = delete;

  double ZValue = -2.3;
  double Tolerance = 0.6;

  // Threshold for step detection
  double ZChangeMin = 0.1;
  // Interactive box pose (center and absolute lengths)
  double BoxPosition[3] = { -10.0, 5.0, -3.0 };
  double BoxScale[3] = { 15.0, 25.0, 2.0 };
};

#endif // VTK_CURB_DETECTOR_H
