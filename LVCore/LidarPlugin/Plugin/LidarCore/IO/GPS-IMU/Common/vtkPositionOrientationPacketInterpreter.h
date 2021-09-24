//=========================================================================
// Copyright 2020 Kitware, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

#ifndef VTKPOSITIONORIENTATIONPACKETINTERPRETER_H
#define VTKPOSITIONORIENTATIONPACKETINTERPRETER_H

#include <IO/vtkInterpreter.h>

#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkPolyData.h>

#include "LidarCoreModule.h"

class LIDARCORE_EXPORT vtkPositionOrientationPacketInterpreter : public vtkInterpreter
{
public:
  vtkTypeMacro(vtkPositionOrientationPacketInterpreter, vtkInterpreter)

  vtkSmartPointer<vtkTable> GetRawInformation(){return this->RawInformation;}
  vtkSmartPointer<vtkPolyData> GetPositionOrientation(){return this->PositionOrientation;}

  bool IsNewRawInformation();

  bool IsNewPositionOrientationInformation();

  virtual bool HasPositionOrientationInformation(){ return false;}

  virtual bool HasRawInformation(){ return false;}

  bool IsNewData() override;

  void ResetCurrentData() override;

  virtual void InitArrays() = 0;

  virtual void FillInterpolatorFromPositionOrientation(){}

protected:
  vtkPositionOrientationPacketInterpreter() = default;

  //! Buffer to store Raw informations
  vtkSmartPointer<vtkTable> RawInformation;

  //! Buffer to store Position and orientation informations
  vtkSmartPointer<vtkPolyData> PositionOrientation;

private:
  vtkPositionOrientationPacketInterpreter(const vtkPositionOrientationPacketInterpreter&) = delete;
  void operator=(const vtkPositionOrientationPacketInterpreter&) = delete;

  int SizeRawInformationLastAsk;

  int SizePositionOrientationInformationLastAsk;
};

#endif // VTKPOSITIONORIENTATIONPACKETINTERPRETER_H
