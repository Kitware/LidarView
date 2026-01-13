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

  // ROI bounds
  vtkSetMacro(XMin, double);
  vtkGetMacro(XMin, double);
  vtkSetMacro(XMax, double);
  vtkGetMacro(XMax, double);
  vtkSetMacro(YMin, double);
  vtkGetMacro(YMin, double);
  vtkSetMacro(YMax, double);
  vtkGetMacro(YMax, double);
  vtkSetMacro(ZMin, double);
  vtkGetMacro(ZMin, double);
  vtkSetMacro(ZMax, double);
  vtkGetMacro(ZMax, double);

  void SetXRange(double minVal, double maxVal);
  void SetYRange(double minVal, double maxVal);
  void SetZRange(double minVal, double maxVal);

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
};

#endif // VTK_CURB_DETECTOR_H
