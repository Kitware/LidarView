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
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTransform.h>

vtkStandardNewMacro(vtkNormalizeExternalSensorData);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, SensorTransform, vtkTransform);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, IMUTransform, vtkTransform);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, OdometryTransform, vtkTransform);
vtkCxxSetObjectMacro(vtkNormalizeExternalSensorData, PoseTransform, vtkTransform);

namespace
{
//-----------------------------------------------------------------------------
inline bool ColumnExists(vtkTable* src, const char* name)
{
  return (src && name && *name && src->GetColumnByName(name) != nullptr);
}

//-----------------------------------------------------------------------------
inline const char* GetColumnIfExists(vtkTable* inTable,
  const std::string& column,
  const char* defaultName)
{
  if (!column.empty() && ::ColumnExists(inTable, column.c_str()))
  {
    return column.c_str();
  }
  if (!column.empty())
  {
    return defaultName;
  }
  return nullptr;
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
vtkSmartPointer<vtkAbstractArray> CopyWithScale(vtkTable* src,
  const char* srcName,
  const char* dstName,
  double scale)
{
  if (!src || !srcName || !dstName)
    return nullptr;
  vtkAbstractArray* aa = src->GetColumnByName(srcName);
  if (!aa)
    return nullptr;

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
      out->SetName(dstName);
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
    out->SetName(dstName);
    out->SetNumberOfValues(n);
    for (vtkIdType i = 0; i < n; ++i)
    {
      out->SetValue(i, dataArray->GetTuple1(i) * scale);
    }
    return out;
  }

  // If column is not a vtkDataArray (numeric), ignore it and warn.
  vtkGenericWarningMacro(<< "NormalizeExternalSensorData: column '" << (srcName ? srcName : "")
                         << "' is not numeric (vtkDataArray); ignoring.");
  return nullptr;
}

//-----------------------------------------------------------------------------
void AddWithScaleIfExists(vtkTable* dst,
  vtkTable* src,
  const char* srcName,
  const char* outName,
  double scale)
{
  if (!srcName || !*srcName)
    return;
  if (auto arr = CopyWithScale(src, srcName, outName ? outName : srcName, scale))
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
int vtkNormalizeExternalSensorData::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkNormalizeExternalSensorData::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
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
    double accelScale = ScaleFromAccelUnit(accelUnit);

    const char* ax = ::GetColumnIfExists(inTable, this->IMUAccXColumn, "acc_x");
    const char* ay = ::GetColumnIfExists(inTable, this->IMUAccYColumn, "acc_y");
    const char* az = ::GetColumnIfExists(inTable, this->IMUAccZColumn, "acc_z");

    AddWithScaleIfExists(output, inTable, ax, "acc_x", accelScale);
    AddWithScaleIfExists(output, inTable, ay, "acc_y", accelScale);
    AddWithScaleIfExists(output, inTable, az, "acc_z", accelScale);

    // Gyro (to rad/s)
    auto gyroUnit = static_cast<vtkNormalizeExternalSensorData::GyroUnit>(this->IMUGyroUnits);
    double gyroScale = ScaleFromGyroUnit(gyroUnit);

    const char* wx = ::GetColumnIfExists(inTable, this->IMUGyroXColumn, "w_x");
    const char* wy = ::GetColumnIfExists(inTable, this->IMUGyroYColumn, "w_y");
    const char* wz = ::GetColumnIfExists(inTable, this->IMUGyroZColumn, "w_z");

    AddWithScaleIfExists(output, inTable, wx, "w_x", gyroScale);
    AddWithScaleIfExists(output, inTable, wy, "w_y", gyroScale);
    AddWithScaleIfExists(output, inTable, wz, "w_z", gyroScale);
  }

  if (this->UseGNSS)
  {
    // Position only (to meters)
    auto posUnit =
      static_cast<vtkNormalizeExternalSensorData::DistanceUnit>(this->GNSSPositionUnits);
    double posScale = ScaleFromDistUnit(posUnit);

    const char* px = ::GetColumnIfExists(inTable, this->GNSSXColumn, "X");
    const char* py = ::GetColumnIfExists(inTable, this->GNSSYColumn, "Y");
    const char* pz = ::GetColumnIfExists(inTable, this->GNSSZColumn, "Z");

    AddWithScaleIfExists(output, inTable, px, "X", posScale);
    AddWithScaleIfExists(output, inTable, py, "Y", posScale);
    AddWithScaleIfExists(output, inTable, pz, "Z", posScale);
  }

  if (this->UseINS)
  {
    // Position (to meters)
    auto posUnit =
      static_cast<vtkNormalizeExternalSensorData::DistanceUnit>(this->GNSSPositionUnits);
    double posScale = ScaleFromDistUnit(posUnit);

    const char* px = ::GetColumnIfExists(inTable, this->GNSSXColumn, "X");
    const char* py = ::GetColumnIfExists(inTable, this->GNSSYColumn, "Y");
    const char* pz = ::GetColumnIfExists(inTable, this->GNSSZColumn, "Z");

    AddWithScaleIfExists(output, inTable, px, "X", posScale);
    AddWithScaleIfExists(output, inTable, py, "Y", posScale);
    AddWithScaleIfExists(output, inTable, pz, "Z", posScale);

    // Euler angles (to radians)
    auto angUnit = static_cast<vtkNormalizeExternalSensorData::AngleUnit>(this->GNSSEulerUnits);
    double angScale = ScaleFromAngleUnit(angUnit);

    const char* r = ::GetColumnIfExists(inTable, this->RollColumn, "Rx(Roll)");
    const char* p = ::GetColumnIfExists(inTable, this->PitchColumn, "Ry(Pitch)");
    const char* y = ::GetColumnIfExists(inTable, this->YawColumn, "Rz(Yaw)");

    AddWithScaleIfExists(output, inTable, r, "Rx(Roll)", angScale);
    AddWithScaleIfExists(output, inTable, p, "Ry(Pitch)", angScale);
    AddWithScaleIfExists(output, inTable, y, "Rz(Yaw)", angScale);
  }

  if (this->UseOdometry)
  {
    // Distance (to meters)
    auto odUnit = static_cast<vtkNormalizeExternalSensorData::DistanceUnit>(this->OdometryUnits);
    double odScale = ScaleFromDistUnit(odUnit);

    const char* od = ::GetColumnIfExists(inTable, this->OdometryColumn, "odom");

    AddWithScaleIfExists(output, inTable, od, "odom", odScale);
  }

  // Time column pass-through (no units conversion).
  {
    const char* tname = ::GetColumnIfExists(inTable, this->TimeColumn, "Time");

    AddWithScaleIfExists(output, inTable, tname, "Time", 1.0);
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
