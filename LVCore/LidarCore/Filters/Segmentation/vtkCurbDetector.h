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

#include <string>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersSegmentationModule.h"

/**
 * vtkCurbDetector extracts curb points in real-time from a LiDAR point cloud
 * within a 3D region of interest (interactive box). It detects the left and
 * right curb edges inside the ROI.
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

  ///@{
  /**
   * Step detection thresholds
   */
  vtkSetMacro(ZChangeMin, double);
  vtkGetMacro(ZChangeMin, double);
  ///@}

  ///@{
  /**
   * Number of neighbors on each side (window half-size)
   */
  vtkSetMacro(NeighborCount, int);
  vtkGetMacro(NeighborCount, int);
  ///@}

  ///@{
  /**
   * RANSAC inlier distance threshold (meters)
   */
  vtkSetMacro(RansacDistanceThreshold, double);
  vtkGetMacro(RansacDistanceThreshold, double);
  ///@}

  ///@{
  /**
   * LiDAR spin rate (Hz), used to derive a timestamp continuity threshold
   */
  vtkSetMacro(SpinRateHz, double);
  vtkGetMacro(SpinRateHz, double);
  ///@}

  ///@{
  /**
   * Name of the point-data array that stores the LiDAR ring (laser id) per point.
   */
  vtkSetStdStringFromCharMacro(RingArrayName);
  vtkGetCharFromStdStringMacro(RingArrayName);
  ///@}

  ///@{
  /**
   * Name of the point-data array that stores per-point timestamps.
   */
  vtkSetStdStringFromCharMacro(TimestampArrayName);
  vtkGetCharFromStdStringMacro(TimestampArrayName);
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

  // Threshold for step detection
  double ZChangeMin = 0.05;
  // Neighbor window half-size for Z-change evaluation
  int NeighborCount = 5;
  // RANSAC inlier distance threshold (m)
  double RansacDistanceThreshold = 0.15;
  // LiDAR spin rate (Hz) — e.g., 10 Hz => ~0.1 s per frame
  double SpinRateHz = 10.0;
  // Interactive box pose (min-corner and absolute lengths)
  double BoxPosition[3] = { -10.0, -10.0, -2.5 };
  double BoxScale[3] = { 20.0, 20.0, 1.0 };
  // Names of arrays used for ring id and timestamp
  std::string RingArrayName = "laser_id";
  std::string TimestampArrayName = "timestamp";
};

#endif // VTK_CURB_DETECTOR_H
