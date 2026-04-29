/*=========================================================================

  Program: LidarView
  Module:  vtkNormalizeExternalSensorData.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkNormalizeExternalSensorData_h
#define vtkNormalizeExternalSensorData_h

#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTableAlgorithm.h>
#include <vtkVector.h>

#include <string>

#include "lvFiltersGeneralModule.h" // for export macro

class vtkTable;
class vtkTransform;

/**
 * Filter that normalizes external sensor data (IMU / GNSS / Odometry) in a vtkTable.
 * Performs unit conversions to SI (m/s^2, rad/s, m, rad) based on user settings.
 */
class LVFILTERSGENERAL_EXPORT vtkNormalizeExternalSensorData : public vtkTableAlgorithm
{
public:
  static vtkNormalizeExternalSensorData* New();
  vtkTypeMacro(vtkNormalizeExternalSensorData, vtkTableAlgorithm);

  ///@{
  /**
   * Enable or disable IMU processing. When disabled, IMU columns ignored.
   */
  vtkSetMacro(UseIMU, bool);
  vtkGetMacro(UseIMU, bool);
  vtkBooleanMacro(UseIMU, bool);
  ///@}

  ///@{
  /**
   * Enable or disable GNSS processing. When disabled, GNSS columns ignored.
   */
  vtkSetMacro(UseGNSS, bool);
  vtkGetMacro(UseGNSS, bool);
  vtkBooleanMacro(UseGNSS, bool);
  ///@}

  ///@{
  /**
   * Enable or disable INS/external pose processing. When disabled, INS columns ignored.
   */
  vtkSetMacro(UseINS, bool);
  vtkGetMacro(UseINS, bool);
  vtkBooleanMacro(UseINS, bool);
  ///@}

  ///@{
  /**
   * Enable or disable INS/GNSS confidence errors columns.
   */
  vtkSetMacro(UseConfidenceError, bool);
  vtkGetMacro(UseConfidenceError, bool);
  vtkBooleanMacro(UseConfidenceError, bool);
  ///@}

  ///@{
  /**
   * Enable or disable Odometry processing. When disabled, odometry column ignored.
   */
  vtkSetMacro(UseOdometry, bool);
  vtkGetMacro(UseOdometry, bool);
  vtkBooleanMacro(UseOdometry, bool);
  ///@}

  ///@{
  /**
   * IMU acceleration units (converted to m/s^2): 1. m/s^2, 2. g, 3. mg.
   */
  enum class AccelUnit
  {
    MPS2 = 1,
    G = 2,
    MG = 3
  };
  vtkSetClampMacro(IMUAccelerationUnits,
    int,
    static_cast<int>(AccelUnit::MPS2),
    static_cast<int>(AccelUnit::MG));
  vtkGetMacro(IMUAccelerationUnits, int);
  ///@}

  ///@{
  /**
   * IMU angular rate units (converted to rad/s): 1. rad/s, 2. deg/s.
   */
  enum class GyroUnit
  {
    RAD_S = 1,
    DEG_S = 2
  };
  vtkSetClampMacro(IMUGyroUnits,
    int,
    static_cast<int>(GyroUnit::RAD_S),
    static_cast<int>(GyroUnit::DEG_S));
  vtkGetMacro(IMUGyroUnits, int);
  ///@}

  ///@{
  /**
   * GNSS position units (converted to meters): 1. m, 2. cm, 3. mm.
   */
  enum class DistanceUnit
  {
    M = 1,
    CM = 2,
    MM = 3
  };
  vtkSetClampMacro(GNSSPositionUnits,
    int,
    static_cast<int>(DistanceUnit::M),
    static_cast<int>(DistanceUnit::MM));
  vtkGetMacro(GNSSPositionUnits, int);
  ///@}

  ///@{
  /**
   * GNSS Euler units (converted to radians): 1. rad, 2. deg.
   */
  enum class AngleUnit
  {
    RAD = 1,
    DEG = 2
  };
  vtkSetClampMacro(GNSSEulerUnits,
    int,
    static_cast<int>(AngleUnit::RAD),
    static_cast<int>(AngleUnit::DEG));
  vtkGetMacro(GNSSEulerUnits, int);
  ///@}

  ///@{
  /**
   * Odometry units (converted to meters): 1. m, 2. cm, 3. mm.
   */
  vtkSetClampMacro(OdometryUnits,
    int,
    static_cast<int>(DistanceUnit::M),
    static_cast<int>(DistanceUnit::MM));
  vtkGetMacro(OdometryUnits, int);
  ///@}

  ///@{
  /**
   * IMU acceleration X column name (maps to acc_x).
   */
  vtkSetMacro(IMUAccXColumn, std::string);
  vtkGetMacro(IMUAccXColumn, std::string);
  ///@}

  ///@{
  /**
   * IMU acceleration Y column name (maps to acc_y).
   */
  vtkSetMacro(IMUAccYColumn, std::string);
  vtkGetMacro(IMUAccYColumn, std::string);
  ///@}

  ///@{
  /**
   * IMU acceleration Z column name (maps to acc_z).
   */
  vtkSetMacro(IMUAccZColumn, std::string);
  vtkGetMacro(IMUAccZColumn, std::string);
  ///@}

  ///@{
  /**
   * IMU angular rate X column name (maps to w_x).
   */
  vtkSetMacro(IMUGyroXColumn, std::string);
  vtkGetMacro(IMUGyroXColumn, std::string);
  ///@}

  ///@{
  /**
   * IMU angular rate Y column name (maps to w_y).
   */
  vtkSetMacro(IMUGyroYColumn, std::string);
  vtkGetMacro(IMUGyroYColumn, std::string);
  ///@}

  ///@{
  /**
   * IMU angular rate Z column name (maps to w_z).
   */
  vtkSetMacro(IMUGyroZColumn, std::string);
  vtkGetMacro(IMUGyroZColumn, std::string);
  ///@}

  ///@{
  /**
   * GNSS X position column name (maps to X).
   */
  vtkSetMacro(GNSSXColumn, std::string);
  vtkGetMacro(GNSSXColumn, std::string);
  ///@}

  ///@{
  /**
   * GNSS Y position column name (maps to Y).
   */
  vtkSetMacro(GNSSYColumn, std::string);
  vtkGetMacro(GNSSYColumn, std::string);
  ///@}

  ///@{
  /**
   * GNSS Z position column name (maps to Z).
   */
  vtkSetMacro(GNSSZColumn, std::string);
  vtkGetMacro(GNSSZColumn, std::string);
  ///@}

  ///@{
  /**
   * Roll (Rx) column name (maps to Rx/roll).
   */
  vtkSetMacro(RollColumn, std::string);
  vtkGetMacro(RollColumn, std::string);
  ///@}

  ///@{
  /**
   * Pitch (Ry) column name (maps to Ry/pitch).
   */
  vtkSetMacro(PitchColumn, std::string);
  vtkGetMacro(PitchColumn, std::string);
  ///@}

  ///@{
  /**
   * Yaw (Rz) column name (maps to Rz/yaw).
   */
  vtkSetMacro(YawColumn, std::string);
  vtkGetMacro(YawColumn, std::string);
  ///@}

  ///@{
  /**
   * GNSS X position error column name.
   */
  vtkSetMacro(GNSSXErrorColumn, std::string);
  vtkGetMacro(GNSSXErrorColumn, std::string);
  ///@}

  ///@{
  /**
   * GNSS Y position error column name.
   */
  vtkSetMacro(GNSSYErrorColumn, std::string);
  vtkGetMacro(GNSSYErrorColumn, std::string);
  ///@}

  ///@{
  /**
   * GNSS Z position error column name.
   */
  vtkSetMacro(GNSSZErrorColumn, std::string);
  vtkGetMacro(GNSSZErrorColumn, std::string);
  ///@}

  ///@{
  /**
   * Roll (Rx) error column name.
   */
  vtkSetMacro(RollErrorColumn, std::string);
  vtkGetMacro(RollErrorColumn, std::string);
  ///@}

  ///@{
  /**
   * Pitch (Ry) error column name.
   */
  vtkSetMacro(PitchErrorColumn, std::string);
  vtkGetMacro(PitchErrorColumn, std::string);
  ///@}

  ///@{
  /**
   * Yaw (Rz) error column name.
   */
  vtkSetMacro(YawErrorColumn, std::string);
  vtkGetMacro(YawErrorColumn, std::string);
  ///@}

  ///@{
  /**
   * Odometry column name (maps to odom).
   */
  vtkSetMacro(OdometryColumn, std::string);
  vtkGetMacro(OdometryColumn, std::string);
  ///@}

  ///@{
  /**
   * Timestamp column name (used to synchronize/merge streams).
   */
  vtkSetMacro(TimeColumn, std::string);
  vtkGetMacro(TimeColumn, std::string);
  ///@}

  /**
   * Utility for UI to list the available input table column names.
   */
  vtkStringArray* GetHeaderColumns();

  /**
   * Expose modification time so upstream readers can track pipeline updates.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Transform setters/getters (specific transforms override SensorTransform)
   */
  vtkGetObjectMacro(SensorTransform, vtkTransform);
  virtual void SetSensorTransform(vtkTransform*);
  vtkGetObjectMacro(IMUTransform, vtkTransform);
  virtual void SetIMUTransform(vtkTransform*);
  vtkGetObjectMacro(OdometryTransform, vtkTransform);
  virtual void SetOdometryTransform(vtkTransform*);
  vtkGetObjectMacro(PoseTransform, vtkTransform);
  virtual void SetPoseTransform(vtkTransform*);
  ///@}

  ///@{
  /**
   * Helper functions that append to an existing table external sensor data
   */
  static void AppendIMUDataToTable(vtkTable* out,
    const vtkVector3d& acc,
    const vtkVector3d& gyro,
    AccelUnit accUnit = AccelUnit::MPS2,
    GyroUnit gyroUnit = GyroUnit::RAD_S);
  static void AppendGNSSDataToTable(vtkTable* out,
    const vtkVector3d& position,
    DistanceUnit positionUnit = DistanceUnit::M);
  static void AppendINSDataToTable(vtkTable* out,
    const vtkVector3d& position,
    const vtkVector3d& angles,
    DistanceUnit positionUnit = DistanceUnit::M,
    AngleUnit angleUnit = AngleUnit::RAD);
  static void AppendOdometryDataToTable(vtkTable* out,
    double distance,
    DistanceUnit distanecUnit = DistanceUnit::M);
  static void AppendTimeDataToTable(vtkTable* out, double timestamp);
  ///@}

  /**
   * Set the matrix 4x4 transform in the field data of a vtkTable.
   */
  static void SetTransformInFieldData(vtkTable* out,
    vtkTransform* transform,
    const std::string& name);

  ///@{
  /**
   * Array name used in SLAM for the supported external sensors.
   */
  static const char* IMU_ACC_X_ARRAY_NAME() { return "acc_x"; }
  static const char* IMU_ACC_Y_ARRAY_NAME() { return "acc_y"; }
  static const char* IMU_ACC_Z_ARRAY_NAME() { return "acc_z"; }
  static const char* IMU_GYRO_X_ARRAY_NAME() { return "w_x"; }
  static const char* IMU_GYRO_Y_ARRAY_NAME() { return "w_y"; }
  static const char* IMU_GYRO_Z_ARRAY_NAME() { return "w_z"; }
  static const char* GNSS_POS_X_ARRAY_NAME() { return "X"; }
  static const char* GNSS_POS_Y_ARRAY_NAME() { return "Y"; }
  static const char* GNSS_POS_Z_ARRAY_NAME() { return "Z"; }
  static const char* INS_ANGLE_RX_ARRAY_NAME() { return "Rx(Roll)"; }
  static const char* INS_ANGLE_RY_ARRAY_NAME() { return "Ry(Pitch)"; }
  static const char* INS_ANGLE_RZ_ARRAY_NAME() { return "Rz(Yaw)"; }
  static const char* GNSS_POS_X_ERROR_ARRAY_NAME() { return "errX"; }
  static const char* GNSS_POS_Y_ERROR_ARRAY_NAME() { return "errY"; }
  static const char* GNSS_POS_Z_ERROR_ARRAY_NAME() { return "errZ"; }
  static const char* INS_ANGLE_RX_ERROR_ARRAY_NAME() { return "errRoll"; }
  static const char* INS_ANGLE_RY_ERROR_ARRAY_NAME() { return "errPitch"; }
  static const char* INS_ANGLE_RZ_ERROR_ARRAY_NAME() { return "errYaw"; }
  static const char* ODOMETRY_DISTANCE_ARRAY_NAME() { return "odom"; }
  static const char* TIME_ARRAY_NAME() { return "Time"; }
  static const char* CALIBRATION_IMU_NAME() { return "Calibration_IMU"; }
  static const char* CALIBRATION_GNSS_NAME() { return "Calibration_GNSS"; }
  static const char* CALIBRATION_INS_NAME() { return "Calibration_INS"; }
  static const char* CALIBRATION_ODOMETRY_NAME() { return "Calibration_Odometry"; }
  ///@}

  /**
   * Array name used in SLAM to synchronize LiDAR & external sensors.
   */
  static const char* TIME_SYNC_FIELD_DATA_ARRAY_NAME() { return "Timestamp"; }

protected:
  vtkNormalizeExternalSensorData() = default;
  ~vtkNormalizeExternalSensorData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkNormalizeExternalSensorData(const vtkNormalizeExternalSensorData&) = delete;
  void operator=(const vtkNormalizeExternalSensorData&) = delete;

  bool UseIMU = false;
  bool UseGNSS = false;
  bool UseINS = false;
  bool UseOdometry = false;
  bool UseConfidenceError = false;

  int IMUAccelerationUnits = 1;
  int IMUGyroUnits = 1;
  int GNSSPositionUnits = 1;
  int GNSSEulerUnits = 1;
  int OdometryUnits = 1;

  std::string IMUAccXColumn;
  std::string IMUAccYColumn;
  std::string IMUAccZColumn;

  std::string IMUGyroXColumn;
  std::string IMUGyroYColumn;
  std::string IMUGyroZColumn;

  std::string GNSSXColumn;
  std::string GNSSYColumn;
  std::string GNSSZColumn;

  std::string GNSSXErrorColumn;
  std::string GNSSYErrorColumn;
  std::string GNSSZErrorColumn;

  std::string RollColumn;
  std::string PitchColumn;
  std::string YawColumn;

  std::string RollErrorColumn;
  std::string YawErrorColumn;
  std::string PitchErrorColumn;

  std::string OdometryColumn;

  std::string TimeColumn;

  vtkSmartPointer<vtkStringArray> HeaderColumnsCacheArray;

  // Optional transform for GNSS positions / IMU vectors and Odometry
  vtkTransform* SensorTransform = nullptr;

  // Optional per-sensor transforms
  vtkTransform* IMUTransform = nullptr;
  vtkTransform* OdometryTransform = nullptr;
  vtkTransform* PoseTransform = nullptr;
};

#endif // vtkNormalizeExternalSensorData_h
