/*=========================================================================

  Program: LidarView
  Module:  vtkPosePacketInterpreter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPosePacketInterpreter_h
#define vtkPosePacketInterpreter_h

#include "vtkInterpreter.h"

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include "lvIOLidarModule.h"

class LVIOLIDAR_EXPORT vtkPosePacketInterpreter : public vtkInterpreter
{
public:
  vtkTypeMacro(vtkPosePacketInterpreter, vtkInterpreter)

  vtkSmartPointer<vtkTable> GetRawInformation() { return this->RawInformation; }
  vtkSmartPointer<vtkPolyData> GetPose() { return this->Pose; }

  bool IsNewRawInformation();

  bool IsNewPoseInformation();

  virtual bool HasPoseInformation() { return false; }

  virtual bool HasRawInformation() { return false; }

  bool IsNewData() override;

  void ResetCurrentData() override;

  virtual void InitArrays() = 0;

  virtual void FillInterpolatorFromPose() {}

protected:
  vtkPosePacketInterpreter() = default;

  //! Buffer to store Raw informations
  vtkSmartPointer<vtkTable> RawInformation;

  //! Buffer to store Position and orientation informations
  vtkSmartPointer<vtkPolyData> Pose;

private:
  vtkPosePacketInterpreter(const vtkPosePacketInterpreter&) = delete;
  void operator=(const vtkPosePacketInterpreter&) = delete;

  int SizeRawInformationLastAsk = 0;

  int SizePoseInformationLastAsk = 0;
};

#endif
