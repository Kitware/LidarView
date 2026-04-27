/*=========================================================================

  Program: LidarView
  Module:  vtkINSTableToPoses.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkINSTableToPoses.h"

#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkNormalizeExternalSensorData.h>
#include <vtkPointData.h>
#include <vtkPolyLine.h>
#include <vtkSMPTools.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkTableToPolyData.h>
#include <vtkTransform.h>

#include <numeric>

#include "vtk_eigen.h"
#include VTK_EIGEN(Eigen)

using vtkNESD = vtkNormalizeExternalSensorData;
#include <iostream>
namespace
{
//------------------------------------------------------------------------------
Eigen::Matrix3d RPYtoRotationMatrix(double roll, double pitch, double yaw)
{
  return Eigen::Matrix3d(Eigen::AngleAxisd(yaw, Eigen::Vector3d::UnitZ()) *
    Eigen::AngleAxisd(pitch, Eigen::Vector3d::UnitY()) *
    Eigen::AngleAxisd(roll, Eigen::Vector3d::UnitX()));
}

//------------------------------------------------------------------------------
Eigen::Vector3d RotationMatrixToRPY(const Eigen::Matrix3d& rot)
{
  Eigen::Vector3d rpy;
  rpy.x() = std::atan2(rot(2, 1), rot(2, 2));
  rpy.y() = -std::asin(rot(2, 0));
  rpy.z() = std::atan2(rot(1, 0), rot(0, 0));
  return rpy;
}

//------------------------------------------------------------------------------
void ConvertAndTransformPoints(vtkTable* table, vtkPolyData* poses)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(table->GetNumberOfRows());
  poses->SetPoints(points);

  std::vector<vtkIdType> ptIds(table->GetNumberOfRows());
  std::iota(ptIds.begin(), ptIds.end(), 0);
  poses->AllocateEstimate(1, 1);
  poses->InsertNextCell(VTK_POLY_VERTEX, table->GetNumberOfRows(), ptIds.data());
  for (int cc = 0; cc < table->GetNumberOfColumns(); cc++)
  {
    vtkAbstractArray* arr = table->GetColumn(cc);
    std::string name = table->GetColumnName(cc);
    if (name != vtkNESD::GNSS_POS_X_ARRAY_NAME() && name != vtkNESD::GNSS_POS_Y_ARRAY_NAME() &&
      name != vtkNESD::GNSS_POS_Z_ARRAY_NAME())
    {
      poses->GetPointData()->AddArray(arr);
    }
  }

  vtkDoubleArray* array = vtkArrayDownCast<vtkDoubleArray>(
    table->GetFieldData()->GetAbstractArray(vtkNESD::CALIBRATION_INS_NAME()));
  Eigen::Matrix4d matrix;
  for (int r = 0; r < 4; ++r)
  {
    for (int c = 0; c < 4; ++c)
    {
      matrix(r, c) = array->GetComponent(r, c);
    }
  }
  // The pose of INS sensor origin in base coordinate system
  Eigen::Isometry3d sensorToBase(matrix);

  vtkDataArray* x =
    vtkArrayDownCast<vtkDataArray>(table->GetColumnByName(vtkNESD::GNSS_POS_X_ARRAY_NAME()));
  vtkDataArray* y =
    vtkArrayDownCast<vtkDataArray>(table->GetColumnByName(vtkNESD::GNSS_POS_Y_ARRAY_NAME()));
  vtkDataArray* z =
    vtkArrayDownCast<vtkDataArray>(table->GetColumnByName(vtkNESD::GNSS_POS_Z_ARRAY_NAME()));
  vtkDataArray* roll = vtkArrayDownCast<vtkDataArray>(
    poses->GetPointData()->GetAbstractArray(vtkNESD::INS_ANGLE_RX_ARRAY_NAME()));
  vtkDataArray* pitch = vtkArrayDownCast<vtkDataArray>(
    poses->GetPointData()->GetAbstractArray(vtkNESD::INS_ANGLE_RY_ARRAY_NAME()));
  vtkDataArray* yaw = vtkArrayDownCast<vtkDataArray>(
    poses->GetPointData()->GetAbstractArray(vtkNESD::INS_ANGLE_RZ_ARRAY_NAME()));
  auto transformPoints = [&](vtkIdType begin, vtkIdType end)
  {
    for (vtkIdType idx = begin; idx < end; ++idx)
    {
      Eigen::Isometry3d pose;
      pose.linear() =
        ::RPYtoRotationMatrix(roll->GetTuple1(idx), pitch->GetTuple1(idx), yaw->GetTuple1(idx));
      pose.translation() = Eigen::Vector3d(x->GetTuple1(idx), y->GetTuple1(idx), z->GetTuple1(idx));
      pose.makeAffine();
      // Transform external sensor pose into base reference
      Eigen::Isometry3d newPose = sensorToBase * pose;
      Eigen::Vector3d point = newPose.translation();
      Eigen::Vector3d orientation = ::RotationMatrixToRPY(newPose.linear());
      points->SetPoint(idx, point.data());
      roll->SetTuple1(idx, orientation.x());
      pitch->SetTuple1(idx, orientation.y());
      yaw->SetTuple1(idx, orientation.z());
    }
  };
  vtkSMPTools::For(0, table->GetNumberOfRows(), transformPoints);
}
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkINSTableToPoses);

//----------------------------------------------------------------------------
int vtkINSTableToPoses::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int vtkINSTableToPoses::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get IO
  vtkTable* insTable = vtkTable::GetData(inputVector[0]);
  if (!insTable)
  {
    vtkErrorMacro(<< "Invalid vtkTable input!");
    return 0;
  }

  if (!insTable->GetColumnByName(vtkNESD::GNSS_POS_X_ARRAY_NAME()) ||
    !insTable->GetColumnByName(vtkNESD::GNSS_POS_Y_ARRAY_NAME()) ||
    !insTable->GetColumnByName(vtkNESD::GNSS_POS_Z_ARRAY_NAME()))
  {
    vtkErrorMacro(<< "Missing X, Y or Z column. Cannot convert to point!");
    return 0;
  }
  vtkNew<vtkPolyData> poses;
  vtkAbstractArray* calibArray =
    insTable->GetFieldData()->GetAbstractArray(vtkNESD::CALIBRATION_INS_NAME());
  vtkNew<vtkTableToPolyData> tableToPoint;
  if (calibArray)
  {
    if (!insTable->GetColumnByName(vtkNESD::INS_ANGLE_RX_ARRAY_NAME()) ||
      !insTable->GetColumnByName(vtkNESD::INS_ANGLE_RY_ARRAY_NAME()) ||
      !insTable->GetColumnByName(vtkNESD::INS_ANGLE_RZ_ARRAY_NAME()))
    {
      vtkErrorMacro(<< "Missing Roll, Pitch or Yaw column. Cannot transform pose!");
      return 0;
    }
    if (calibArray->GetNumberOfComponents() != 4 || calibArray->GetNumberOfTuples() != 4)
    {
      vtkErrorMacro("Gps to base transform is not a 4*4 matrix!");
      return 0;
    }
    ::ConvertAndTransformPoints(insTable, poses);
  }
  else
  {
    tableToPoint->SetInputData(insTable);
    tableToPoint->SetXColumn(vtkNESD::GNSS_POS_X_ARRAY_NAME());
    tableToPoint->SetYColumn(vtkNESD::GNSS_POS_Y_ARRAY_NAME());
    tableToPoint->SetZColumn(vtkNESD::GNSS_POS_Z_ARRAY_NAME());
    tableToPoint->Update();
    poses->ShallowCopy(tableToPoint->GetOutput());
  }

  if (!poses)
  {
    vtkErrorMacro("Invalid poses, something has gone wrong!");
    return 0;
  }

  // Create the cell in the same time for visualization
  auto polyLine = vtkSmartPointer<vtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(poses->GetNumberOfPoints());
  for (vtkIdType i = 0; i < poses->GetNumberOfPoints(); i++)
  {
    polyLine->GetPointIds()->SetId(i, i);
  }
  auto cell = vtkSmartPointer<vtkCellArray>::New();
  cell->InsertNextCell(polyLine);
  poses->SetLines(cell);

  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);
  output->ShallowCopy(poses);
  return 1;
}
