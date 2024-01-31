/*=========================================================================

  Program: LidarView
  Module:  vtkPosePacketInterpreter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPosePacketInterpreter.h"

//------------------------------------------------------------------------------
void vtkPosePacketInterpreter::ResetCurrentData()
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
bool vtkPosePacketInterpreter::IsNewRawInformation()
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
bool vtkPosePacketInterpreter::IsNewPoseInformation()
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
bool vtkPosePacketInterpreter::IsNewData()
{
  return (this->IsNewRawInformation() || this->IsNewPoseInformation());
}
