/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPosePacketInterpreter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarPosePacketInterpreter.h"

//------------------------------------------------------------------------------
void vtkLidarPosePacketInterpreter::ResetCurrentData()
{
  if (this->HasPoseInformation())
  {
    this->Pose = vtkSmartPointer<vtkPolyData>::New();
  }

  if (this->HasRawInformation())
  {
    this->RawInformation = vtkTable::New();
  }
  this->InitArrays();
}

//------------------------------------------------------------------------------
bool vtkLidarPosePacketInterpreter::IsNewRawInformation()
{
  if (!this->HasRawInformation())
  {
    return false;
  }
  bool isNewRaw = this->SizeRawInformationLastAsk != this->RawInformation->GetNumberOfRows();
  this->SizeRawInformationLastAsk = this->RawInformation->GetNumberOfRows();
  return isNewRaw;
}

//------------------------------------------------------------------------------
bool vtkLidarPosePacketInterpreter::IsNewPoseInformation()
{
  if (!this->HasPoseInformation())
  {
    return false;
  }
  bool isNewPos = this->SizePoseInformationLastAsk !=
    this->Pose->GetNumberOfPoints();
  this->SizePoseInformationLastAsk = this->Pose->GetNumberOfPoints();
  return isNewPos;
}

//------------------------------------------------------------------------------
bool vtkLidarPosePacketInterpreter::IsNewData()
{
  return (this->IsNewRawInformation() || this->IsNewPoseInformation());
}
