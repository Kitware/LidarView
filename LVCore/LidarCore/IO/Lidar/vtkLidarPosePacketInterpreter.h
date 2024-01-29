/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPosePacketInterpreter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLidarPosePacketInterpreter_h
#define vtkLidarPosePacketInterpreter_h

#include "vtkInterpreter.h"

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include "lvIOLidarModule.h"

class LVIOLIDAR_EXPORT vtkLidarPosePacketInterpreter : public vtkInterpreter
{
public:
  vtkTypeMacro(vtkLidarPosePacketInterpreter, vtkInterpreter)

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
  vtkLidarPosePacketInterpreter() = default;

  //! Buffer to store Raw informations
  vtkSmartPointer<vtkTable> RawInformation;

  //! Buffer to store Position and orientation informations
  vtkSmartPointer<vtkPolyData> Pose;

private:
  vtkLidarPosePacketInterpreter(const vtkLidarPosePacketInterpreter&) = delete;
  void operator=(const vtkLidarPosePacketInterpreter&) = delete;

  int SizeRawInformationLastAsk = 0;

  int SizePoseInformationLastAsk = 0;
};

#endif
