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
#include <string>
#include <vtkTableAlgorithm.h>

#include "lvFiltersCalibrationModule.h"

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
   * Voxel grid leaf size (meters).
   */
  vtkSetMacro(VoxelLeafSize, double);
  vtkGetMacro(VoxelLeafSize, double);
  ///@}

  ///@{
  /**
   * Offline aggregation controls: use all frames or a subrange [FirstFrame, LastFrame] with
   * StepSize.
   */
  vtkSetMacro(AllFrames, bool);
  vtkGetMacro(AllFrames, bool);
  vtkSetMacro(FirstFrame, int);
  vtkGetMacro(FirstFrame, int);
  vtkSetMacro(LastFrame, int);
  vtkGetMacro(LastFrame, int);
  vtkSetMacro(StepSize, int);
  vtkGetMacro(StepSize, int);
  ///@}

protected:
  vtkCalibrationEvaluation();
  ~vtkCalibrationEvaluation() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCalibrationEvaluation(const vtkCalibrationEvaluation&) = delete;
  void operator=(const vtkCalibrationEvaluation&) = delete;

  double AngleRangeDeg[3] = { 0., 0., 0. };
  double TranslationRangeM[3] = { 0., 0., 0. };
  int AngleSteps[3] = { 1, 1, 1 };
  int TranslationSteps[3] = { 1, 1, 1 };

  // Forwarded properties defaults
  bool AllFrames = false;
  bool AutoComputeBounds = true;
  bool AutoDetectTimeArray = true;
  bool IsVoxelGridFilterUsed = true;
  bool AutoDetectTimeUnitConversion = true;

  int FirstFrame = 100;
  int LastFrame = 150;
  int StepSize = 10;
  int VoxelSamplingMode = 0;
  int InterpolationType = 0; // linear
  double VoxelLeafSize = 0.05;
  double CustomConversionFactorToSecond = 1e-6;
  double TimeOffset = 0.0;
};

#endif // VTK_CALIBRATION_EVALUATE_H
