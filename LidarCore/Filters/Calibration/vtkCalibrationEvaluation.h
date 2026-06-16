/*=========================================================================

  Program: LidarView
  Module:  vtkCalibrationEvaluation.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_CALIBRATION_EVALUATE_H
#define VTK_CALIBRATION_EVALUATE_H

// VTK
#include <vtkSmartPointer.h>
#include <vtkTableAlgorithm.h>

#include "lvFiltersCalibrationModule.h"

class vtkAggregatePointsFromTrajectoryOffline;

/**
 * Brute-force evaluator for INS/LiDAR extrinsic calibration.
 *
 * Automatically retrieves the upstream SensorTransform (calibration)
 * as the initial pose, then sweeps around it over user-defined
 * angle and translation ranges. For each candidate, the LiDAR data
 * is transformed, aggregated along the INS trajectory, and the
 * occupied voxel count is used as a calibration quality metric.
 *
 * Inputs:
 *   - Port 0: LiDAR point cloud (vtkPolyData)
 *   - Port 1: INS trajectory (vtkPolyData)
 *
 * Output:
 *   - vtkTable with columns: roll_deg, pitch_deg, yaw_deg, tx_m, ty_m, tz_m, voxels, voxels_rank
 */
class LVFILTERSCALIBRATION_EXPORT vtkCalibrationEvaluation : public vtkTableAlgorithm
{
public:
  static vtkCalibrationEvaluation* New();
  vtkTypeMacro(vtkCalibrationEvaluation, vtkTableAlgorithm)

  /**
   * Aggregator subproxy setter. When provided via ServerManager
   * SubProxy, the evaluation reuses this instance across candidates and
   * calls Clear() between iterations instead of creating a new aggregator
   * every time.
   */
  void SetAggregator(vtkAggregatePointsFromTrajectoryOffline* agg);

  ///@{
  /**
   * Whether to use upstream SensorTransform as the sweep center when available.
   * If disabled, uses the user-provided Base Position/Rotation.
   */
  vtkSetMacro(UseUpstreamCalibration, bool);
  vtkGetMacro(UseUpstreamCalibration, bool);
  ///@}

  ///@{
  /**
   * Angle search half-range in degrees (Roll, Pitch, Yaw).
   * The sweep spans [-range, +range] on each enabled axis.
   */
  vtkSetVector3Macro(AngleRangeDeg, double);
  vtkGetVector3Macro(AngleRangeDeg, double);
  ///@}

  ///@{
  /**
   * Translation search half-range in meters (X, Y, Z).
   * The sweep spans [-range, +range] on each enabled axis.
   */
  vtkSetVector3Macro(TranslationRangeM, double);
  vtkGetVector3Macro(TranslationRangeM, double);
  ///@}

  ///@{
  /**
   * Discretization steps for angles (Roll, Pitch, Yaw).
   * Steps <= 1 disables sweep on that axis.
   */
  vtkSetVector3Macro(AngleSteps, int);
  vtkGetVector3Macro(AngleSteps, int);
  ///@}

  ///@{
  /**
   * Discretization steps for translations (X, Y, Z).
   * Steps <= 1 disables sweep on that axis.
   */
  vtkSetVector3Macro(TranslationSteps, int);
  vtkGetVector3Macro(TranslationSteps, int);
  ///@}

  ///@{
  /**
   * Base transform used as the center of the sweep, in meters and degrees.
   * User-editable. Upstream values, if any, are exposed separately as information-only
   * properties and can be used when UseUpstreamCalibration is enabled.
   */
  void SetBasePosition(double x, double y, double z);
  vtkGetVector3Macro(BasePosition, double);

  void SetBaseRotationDeg(double roll, double pitch, double yaw);
  vtkGetVector3Macro(BaseRotationDeg, double);
  ///@}

  ///@{
  /**
   * Detected upstream calibration pose.
   */
  vtkGetMacro(UpstreamFound, bool);
  vtkGetVector3Macro(UpstreamPosition, double);
  vtkGetVector3Macro(UpstreamRotationDeg, double);
  ///@}

  ///@{
  /**
   * Output of CalibrationEvaluation array name
   */
  static const char* ROLL_ARRAY_NAME() { return "roll_deg"; }
  static const char* PITCH_ARRAY_NAME() { return "pitch_deg"; }
  static const char* YAW_ARRAY_NAME() { return "yaw_deg"; }
  static const char* TRANSLATION_X_ARRAY_NAME() { return "tx_m"; }
  static const char* TRANSLATION_Y_ARRAY_NAME() { return "ty_m"; }
  static const char* TRANSLATION_Z_ARRAY_NAME() { return "tz_m"; }
  static const char* VOXEL_NB_ARRAY_NAME() { return "voxels_nb"; }
  static const char* VOXEL_RANK_ARRAY_NAME() { return "voxels_rank"; }
  ///@}

protected:
  vtkCalibrationEvaluation();
  ~vtkCalibrationEvaluation() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCalibrationEvaluation(const vtkCalibrationEvaluation&) = delete;
  void operator=(const vtkCalibrationEvaluation&) = delete;

  double AngleRangeDeg[3] = { 0., 0., 0. };
  double TranslationRangeM[3] = { 0., 0., 0. };
  int AngleSteps[3] = { 1, 1, 1 };
  int TranslationSteps[3] = { 1, 1, 1 };

  double BasePosition[3] = { 0., 0., 0. };
  double BaseRotationDeg[3] = { 0., 0., 0. };
  bool UseUpstreamCalibration = true;
  bool UpstreamFound = false;
  double UpstreamPosition[3] = { 0., 0., 0. };
  double UpstreamRotationDeg[3] = { 0., 0., 0. };

  // Aggregator instance provided as a subproxy
  vtkSmartPointer<vtkAggregatePointsFromTrajectoryOffline> Aggregator;

  // Observer tag to forward Aggregator ModifiedEvent
  unsigned long AggregatorObserverTag = 0;
};

#endif // VTK_CALIBRATION_EVALUATE_H
