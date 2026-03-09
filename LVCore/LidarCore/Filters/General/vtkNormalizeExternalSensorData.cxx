/*=========================================================================

  Program: LidarView
  Module:  vtkNormalizeExternalSensorData.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNormalizeExternalSensorData.h"

#include <vtkAbstractArray.h>
#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTransform.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkNormalizeExternalSensorData);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, SensorTransform, vtkTransform);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, IMUTransform, vtkTransform);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, OdometryTransform, vtkTransform);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, PoseTransform, vtkTransform);

namespace
{
//-----------------------------------------------------------------------------
inline std::string GetColumnNameIfExists(vtkTable* inTable,
  const std::string& column,
  const std::string& defaultName)
{
  if (inTable && inTable->GetColumnByName(column.c_str()))
  {
    return column;
  }
  if (!column.empty())
  {
    return defaultName;
  }
  return "";
}

//-----------------------------------------------------------------------------
double ScaleFromAccelUnit(vtkNormalizeExternalSensorData::AccelUnit u)
{
  switch (u)
  {
    case vtkNormalizeExternalSensorData::AccelUnit::MPS2:
      return 1.0;
    case vtkNormalizeExternalSensorData::AccelUnit::G:
      return 9.80665;
    case vtkNormalizeExternalSensorData::AccelUnit::MG:
      return 9.80665e-3;
    default:
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
double ScaleFromGyroUnit(vtkNormalizeExternalSensorData::GyroUnit u)
{
  switch (u)
  {
    case vtkNormalizeExternalSensorData::GyroUnit::RAD_S:
      return 1.0;
    case vtkNormalizeExternalSensorData::GyroUnit::DEG_S:
      return vtkMath::Pi() / 180.0;
    default:
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
double ScaleFromDistUnit(vtkNormalizeExternalSensorData::DistanceUnit u)
{
  switch (u)
  {
    case vtkNormalizeExternalSensorData::DistanceUnit::M:
      return 1.0;
    case vtkNormalizeExternalSensorData::DistanceUnit::CM:
      return 0.01;
    case vtkNormalizeExternalSensorData::DistanceUnit::MM:
      return 0.001;
    default:
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
double ScaleFromAngleUnit(vtkNormalizeExternalSensorData::AngleUnit u)
{
  switch (u)
  {
    case vtkNormalizeExternalSensorData::AngleUnit::RAD:
      return 1.0;
    case vtkNormalizeExternalSensorData::AngleUnit::DEG:
      return vtkMath::Pi() / 180.0;
    default:
      return 1.0;
  }
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> CopyColumnWithScale(vtkTable* src,
  const std::string& srcName,
  const std::string& dstName,
  double scale)
{
  if (!src || srcName.empty() || dstName.empty())
  {
    return nullptr;
  }
  vtkAbstractArray* aa = src->GetColumnByName(srcName.c_str());
  if (!aa)
  {
    return nullptr;
  }

  vtkIdType n = aa->GetNumberOfTuples();

  // If column is a numeric array (vtkDataArray or subclass)
  if (auto dataArray = vtkDataArray::SafeDownCast(aa))
  {
    // Identify data type
    const int dtype = dataArray->GetDataType();
    const bool isFloatType = (dtype == VTK_FLOAT || dtype == VTK_DOUBLE);

    // If scaling factor is 1.0 (pure copy), or already float/double type,
    // preserve the original numeric type.
    if (scale == 1.0 || isFloatType)
    {
      vtkSmartPointer<vtkDataArray> out;
      out.TakeReference(vtkDataArray::CreateDataArray(dtype));
      if (!out)
      {
        return nullptr;
      }
      out->SetName(dstName.c_str());
      out->SetNumberOfComponents(1);
      out->SetNumberOfTuples(n);
      // Copy values with scale factor applied
      for (vtkIdType i = 0; i < n; ++i)
      {
        out->SetTuple1(i, dataArray->GetTuple1(i) * scale);
      }
      return out;
    }

    // If input is integer-like and scale is not 1.0,
    // promote to double to preserve fractional values
    vtkSmartPointer<vtkDoubleArray> out = vtkSmartPointer<vtkDoubleArray>::New();
    out->SetName(dstName.c_str());
    out->SetNumberOfValues(n);
    for (vtkIdType i = 0; i < n; ++i)
    {
      out->SetValue(i, dataArray->GetTuple1(i) * scale);
    }
    return out;
  }

  // If column is not a vtkDataArray (numeric), ignore it and warn.
  vtkLog(WARNING,
    << "NormalizeExternalSensorData: column '" << srcName
    << "' is not numeric (vtkDataArray); ignoring.");
  return nullptr;
}

//-----------------------------------------------------------------------------
void CopyColumnWithScaleIfExists(vtkTable* dst,
  vtkTable* src,
  const std::string& srcName,
  const std::string& outName,
  double scale)
{
  if (auto arr = ::CopyColumnWithScale(src, srcName, outName, scale))
  {
    dst->AddColumn(arr);
  }
}
} // namespace

//-----------------------------------------------------------------------------
vtkNormalizeExternalSensorData::~vtkNormalizeExternalSensorData()
{
  this->SetSensorTransform(nullptr);
  this->SetIMUTransform(nullptr);
  this->SetOdometryTransform(nullptr);
  this->SetPoseTransform(nullptr);
}

//-----------------------------------------------------------------------------
vtkStringArray* vtkNormalizeExternalSensorData::GetHeaderColumns()
{
  if (!this->HeaderColumnsCacheArray)
  {
    this->HeaderColumnsCacheArray = vtkSmartPointer<vtkStringArray>::New();
  }
  vtkStringArray* arr = this->HeaderColumnsCacheArray;
  arr->Initialize();

  vtkTable* inTable = vtkTable::SafeDownCast(this->GetInputDataObject(0, 0));
  if (!inTable)
  {
    return arr;
  }

  const int ncols = inTable->GetNumberOfColumns();
  for (int i = 0; i < ncols; ++i)
  {
    vtkAbstractArray* aa = inTable->GetColumn(i);
    const char* name = aa ? aa->GetName() : nullptr;
    if (name && *name)
    {
      arr->InsertNextValue(name);
    }
  }
  return arr;
}

//-----------------------------------------------------------------------------
int vtkNormalizeExternalSensorData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkTable* inTable = vtkTable::GetData(inputVector[0]->GetInformationObject(0));
  vtkTable* output = vtkTable::GetData(outputVector->GetInformationObject(0));
  output->Initialize();

  if (!inTable || inTable->GetNumberOfRows() == 0)
  {
    vtkWarningMacro(<< "Empty or invalid input table");
    return 1;
  }

  if (this->UseIMU)
  {
    // Acceleration (to m/s^2)
    auto accelUnit =
      static_cast<vtkNormalizeExternalSensorData::AccelUnit>(this->IMUAccelerationUnits);
    double accelScale = ::ScaleFromAccelUnit(accelUnit);

    std::string ax = ::GetColumnNameIfExists(inTable, this->IMUAccXColumn, IMU_ACC_X_ARRAY_NAME());
    std::string ay = ::GetColumnNameIfExists(inTable, this->IMUAccYColumn, IMU_ACC_Y_ARRAY_NAME());
    std::string az = ::GetColumnNameIfExists(inTable, this->IMUAccZColumn, IMU_ACC_Z_ARRAY_NAME());

    ::CopyColumnWithScaleIfExists(output, inTable, ax, IMU_ACC_X_ARRAY_NAME(), accelScale);
    ::CopyColumnWithScaleIfExists(output, inTable, ay, IMU_ACC_Y_ARRAY_NAME(), accelScale);
    ::CopyColumnWithScaleIfExists(output, inTable, az, IMU_ACC_Z_ARRAY_NAME(), accelScale);

    // Gyro (to rad/s)
    auto gyroUnit = static_cast<vtkNormalizeExternalSensorData::GyroUnit>(this->IMUGyroUnits);
    double gyroScale = ::ScaleFromGyroUnit(gyroUnit);

    auto wx = ::GetColumnNameIfExists(inTable, this->IMUGyroXColumn, IMU_GYRO_X_ARRAY_NAME());
    auto wy = ::GetColumnNameIfExists(inTable, this->IMUGyroYColumn, IMU_GYRO_Y_ARRAY_NAME());
    auto wz = ::GetColumnNameIfExists(inTable, this->IMUGyroZColumn, IMU_GYRO_Z_ARRAY_NAME());

    ::CopyColumnWithScaleIfExists(output, inTable, wx, IMU_GYRO_X_ARRAY_NAME(), gyroScale);
    ::CopyColumnWithScaleIfExists(output, inTable, wy, IMU_GYRO_Y_ARRAY_NAME(), gyroScale);
    ::CopyColumnWithScaleIfExists(output, inTable, wz, IMU_GYRO_Z_ARRAY_NAME(), gyroScale);
  }

  if (this->UseGNSS)
  {
    // Position only (to meters)
    auto posUnit =
      static_cast<vtkNormalizeExternalSensorData::DistanceUnit>(this->GNSSPositionUnits);
    double posScale = ScaleFromDistUnit(posUnit);

    std::string px = ::GetColumnNameIfExists(inTable, this->GNSSXColumn, GNSS_POS_X_ARRAY_NAME());
    std::string py = ::GetColumnNameIfExists(inTable, this->GNSSYColumn, GNSS_POS_Y_ARRAY_NAME());
    std::string pz = ::GetColumnNameIfExists(inTable, this->GNSSZColumn, GNSS_POS_Z_ARRAY_NAME());

    ::CopyColumnWithScaleIfExists(output, inTable, px, GNSS_POS_X_ARRAY_NAME(), posScale);
    ::CopyColumnWithScaleIfExists(output, inTable, py, GNSS_POS_Y_ARRAY_NAME(), posScale);
    ::CopyColumnWithScaleIfExists(output, inTable, pz, GNSS_POS_Z_ARRAY_NAME(), posScale);
  }

  if (this->UseINS)
  {
    // Position (to meters)
    auto posUnit =
      static_cast<vtkNormalizeExternalSensorData::DistanceUnit>(this->GNSSPositionUnits);
    double posScale = ::ScaleFromDistUnit(posUnit);

    std::string px = ::GetColumnNameIfExists(inTable, this->GNSSXColumn, GNSS_POS_X_ARRAY_NAME());
    std::string py = ::GetColumnNameIfExists(inTable, this->GNSSYColumn, GNSS_POS_Y_ARRAY_NAME());
    std::string pz = ::GetColumnNameIfExists(inTable, this->GNSSZColumn, GNSS_POS_Z_ARRAY_NAME());

    ::CopyColumnWithScaleIfExists(output, inTable, px, GNSS_POS_X_ARRAY_NAME(), posScale);
    ::CopyColumnWithScaleIfExists(output, inTable, py, GNSS_POS_Y_ARRAY_NAME(), posScale);
    ::CopyColumnWithScaleIfExists(output, inTable, pz, GNSS_POS_Z_ARRAY_NAME(), posScale);

    // Euler angles (to radians)
    auto angUnit = static_cast<vtkNormalizeExternalSensorData::AngleUnit>(this->GNSSEulerUnits);
    double angScale = ::ScaleFromAngleUnit(angUnit);

    std::string rx = ::GetColumnNameIfExists(inTable, this->RollColumn, INS_ANGLE_RX_ARRAY_NAME());
    std::string ry = ::GetColumnNameIfExists(inTable, this->PitchColumn, INS_ANGLE_RY_ARRAY_NAME());
    std::string rz = ::GetColumnNameIfExists(inTable, this->YawColumn, INS_ANGLE_RZ_ARRAY_NAME());

    ::CopyColumnWithScaleIfExists(output, inTable, rx, INS_ANGLE_RX_ARRAY_NAME(), angScale);
    ::CopyColumnWithScaleIfExists(output, inTable, ry, INS_ANGLE_RY_ARRAY_NAME(), angScale);
    ::CopyColumnWithScaleIfExists(output, inTable, rz, INS_ANGLE_RZ_ARRAY_NAME(), angScale);
  }

  if (this->UseOdometry)
  {
    // Distance (to meters)
    auto odUnit = static_cast<vtkNormalizeExternalSensorData::DistanceUnit>(this->OdometryUnits);
    double odScale = ::ScaleFromDistUnit(odUnit);

    std::string od =
      ::GetColumnNameIfExists(inTable, this->OdometryColumn, ODOMETRY_DISTANCE_ARRAY_NAME());

    ::CopyColumnWithScaleIfExists(output, inTable, od, ODOMETRY_DISTANCE_ARRAY_NAME(), odScale);
  }

  // Time column pass-through (no units conversion).
  {
    std::string tname = ::GetColumnNameIfExists(inTable, this->TimeColumn, TIME_ARRAY_NAME());

    ::CopyColumnWithScaleIfExists(output, inTable, tname, TIME_ARRAY_NAME(), 1.0);
  }

  if (output->GetNumberOfColumns() == 0)
  {
    vtkWarningMacro(<< "No matching columns found or no sensors enabled; output table is empty.");
  }

  // Propagate field data if any (e.g., calibration matrices from upstream)
  if (inTable && inTable->GetFieldData())
  {
    output->GetFieldData()->ShallowCopy(inTable->GetFieldData());
  }

  // Store selected sensors' calibration transforms into FieldData (4x4 each),
  // matching reader behavior (IMU/GNSS/Odometry use per-sensor transform or fallback to
  // SensorTransform)
  auto storeMatrix = [](vtkTable* tbl, vtkTransform* T, const char* name)
  {
    if (!tbl || !T || !name || !*name)
      return;
    vtkMatrix4x4* M = T->GetMatrix();
    vtkNew<vtkDoubleArray> arr;
    arr->SetName(name);
    arr->SetNumberOfComponents(4);
    arr->SetNumberOfTuples(4);
    for (int r = 0; r < 4; ++r)
    {
      for (int c = 0; c < 4; ++c)
      {
        arr->SetComponent(r, c, M->GetElement(r, c));
      }
    }
    if (auto* fd = tbl->GetFieldData())
    {
      fd->AddArray(arr);
    }
  };

  if (this->UseIMU)
  {
    vtkTransform* t = this->IMUTransform ? this->IMUTransform : this->SensorTransform;
    storeMatrix(output, t, "Calibration_IMU");
  }
  if (this->UseGNSS)
  {
    vtkTransform* t = this->PoseTransform ? this->PoseTransform : this->SensorTransform;
    storeMatrix(output, t, "Calibration_GNSS");
  }
  if (this->UseINS)
  {
    vtkTransform* t = this->PoseTransform ? this->PoseTransform : this->SensorTransform;
    storeMatrix(output, t, "Calibration_INS");
  }
  if (this->UseOdometry)
  {
    vtkTransform* t = this->OdometryTransform ? this->OdometryTransform : this->SensorTransform;
    storeMatrix(output, t, "Calibration_Odometry");
  }

  return 1;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkNormalizeExternalSensorData::GetMTime()
{
  vtkMTimeType mt = this->Superclass::GetMTime();
  if (this->SensorTransform)
  {
    mt = std::max(mt, this->SensorTransform->GetMTime());
  }
  if (this->IMUTransform)
  {
    mt = std::max(mt, this->IMUTransform->GetMTime());
  }
  if (this->PoseTransform)
  {
    mt = std::max(mt, this->PoseTransform->GetMTime());
  }
  if (this->OdometryTransform)
  {
    mt = std::max(mt, this->OdometryTransform->GetMTime());
  }
  return mt;
}
