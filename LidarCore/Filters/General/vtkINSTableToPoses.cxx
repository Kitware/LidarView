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
#include <vtkTemporalTransforms.h>
#include <vtkTransform.h>

using vtkNESD = vtkNormalizeExternalSensorData;

namespace
{
//------------------------------------------------------------------------------
Eigen::Matrix3d RPYtoRotationMatrix(double roll, double pitch, double yaw)
{
  return Eigen::Matrix3d(Eigen::AngleAxisd(yaw, Eigen::Vector3d::UnitZ()) *
    Eigen::AngleAxisd(pitch, Eigen::Vector3d::UnitY()) *
    Eigen::AngleAxisd(roll, Eigen::Vector3d::UnitX()));
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
  if (!insTable->GetColumnByName(vtkNESD::INS_ANGLE_RX_ARRAY_NAME()) ||
    !insTable->GetColumnByName(vtkNESD::INS_ANGLE_RY_ARRAY_NAME()) ||
    !insTable->GetColumnByName(vtkNESD::INS_ANGLE_RZ_ARRAY_NAME()))
  {
    vtkErrorMacro(<< "Missing Roll, Pitch or Yaw column!");
    return 0;
  }
  if (!vtkArrayDownCast<vtkDoubleArray>(insTable->GetColumnByName(vtkNESD::TIME_ARRAY_NAME())))
  {
    vtkErrorMacro(<< "Missing time column!");
    return 0;
  }

  auto axisAngle = vtkSmartPointer<vtkDoubleArray>::New();
  axisAngle->SetNumberOfComponents(4);
  axisAngle->SetNumberOfTuples(insTable->GetNumberOfRows());

  auto position = vtkSmartPointer<vtkDoubleArray>::New();
  position->SetNumberOfComponents(3);
  position->SetNumberOfTuples(insTable->GetNumberOfRows());

  vtkDataArray* x =
    vtkArrayDownCast<vtkDataArray>(insTable->GetColumnByName(vtkNESD::GNSS_POS_X_ARRAY_NAME()));
  vtkDataArray* y =
    vtkArrayDownCast<vtkDataArray>(insTable->GetColumnByName(vtkNESD::GNSS_POS_Y_ARRAY_NAME()));
  vtkDataArray* z =
    vtkArrayDownCast<vtkDataArray>(insTable->GetColumnByName(vtkNESD::GNSS_POS_Z_ARRAY_NAME()));
  vtkDataArray* roll =
    vtkArrayDownCast<vtkDataArray>(insTable->GetColumnByName(vtkNESD::INS_ANGLE_RX_ARRAY_NAME()));
  vtkDataArray* pitch =
    vtkArrayDownCast<vtkDataArray>(insTable->GetColumnByName(vtkNESD::INS_ANGLE_RY_ARRAY_NAME()));
  vtkDataArray* yaw =
    vtkArrayDownCast<vtkDataArray>(insTable->GetColumnByName(vtkNESD::INS_ANGLE_RZ_ARRAY_NAME()));

  vtkAbstractArray* calibArray =
    insTable->GetFieldData()->GetAbstractArray(vtkNESD::CALIBRATION_INS_NAME());
  if (calibArray)
  {
    if (calibArray->GetNumberOfComponents() != 4 || calibArray->GetNumberOfTuples() != 4)
    {
      vtkErrorMacro("Gps to base transform is not a 4*4 matrix!");
      return 0;
    }

    vtkDoubleArray* array = vtkArrayDownCast<vtkDoubleArray>(
      insTable->GetFieldData()->GetAbstractArray(vtkNESD::CALIBRATION_INS_NAME()));
    Eigen::Matrix4d matrix;
    for (int r = 0; r < 4; ++r)
    {
      for (int c = 0; c < 4; ++c)
      {
        matrix(r, c) = array->GetComponent(r, c);
      }
    }
    // Transform sensor data to base coordinate system
    Eigen::Isometry3d sensorToBase(matrix);

    auto transformPoints = [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        Eigen::Isometry3d pose;
        pose.linear() =
          ::RPYtoRotationMatrix(roll->GetTuple1(idx), pitch->GetTuple1(idx), yaw->GetTuple1(idx));
        pose.translation() =
          Eigen::Vector3d(x->GetTuple1(idx), y->GetTuple1(idx), z->GetTuple1(idx));
        pose.makeAffine();
        // Transform external sensor pose into base reference
        Eigen::Isometry3d newPose = sensorToBase * pose;
        Eigen::Vector3d point = newPose.translation();
        Eigen::AngleAxisd angleAxis(newPose.linear());
        position->SetTuple3(idx, point.x(), point.y(), point.z());
        axisAngle->SetTuple4(
          idx, angleAxis.axis()[0], angleAxis.axis()[1], angleAxis.axis()[2], angleAxis.angle());
      }
    };
    vtkSMPTools::For(0, insTable->GetNumberOfRows(), transformPoints);
  }
  else
  {
    auto transformPoints = [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        Eigen::AngleAxisd angleAxis(
          ::RPYtoRotationMatrix(roll->GetTuple1(idx), pitch->GetTuple1(idx), yaw->GetTuple1(idx)));
        position->SetTuple3(idx, x->GetTuple1(idx), y->GetTuple1(idx), z->GetTuple1(idx));
        axisAngle->SetTuple4(
          idx, angleAxis.axis()[0], angleAxis.axis()[1], angleAxis.axis()[2], angleAxis.angle());
      }
    };
    vtkSMPTools::For(0, insTable->GetNumberOfRows(), transformPoints);
  }

  // Create the cell to be able to visualize the data.
  auto trajectory = vtkSmartPointer<vtkTemporalTransforms>::New();
  trajectory->SetTranslationArray(position);
  trajectory->SetTimeArray(
    vtkArrayDownCast<vtkDoubleArray>(insTable->GetColumnByName(vtkNESD::TIME_ARRAY_NAME())));
  trajectory->SetOrientationArray(axisAngle);

  // Set the filter output
  vtkPolyData* output = vtkPolyData::GetData(outputVector);
  output->ShallowCopy(trajectory);
  return 1;
}
