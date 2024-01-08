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

#ifndef vtkLidarPoseStream_h
#define vtkLidarPoseStream_h

#include <memory>

#include <vtkPolyData.h>
#include <vtkTable.h>

#include "vtkStream.h"
#include "vtkLidarPosePacketInterpreter.h"

#include "lvIOLidarModule.h"

class LVIOLIDAR_EXPORT vtkLidarPoseStream : public vtkStream
{
public:
  static vtkLidarPoseStream* New();
  vtkTypeMacro(vtkLidarPoseStream, vtkStream)

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  void AddNewData() override;

  void ClearAllDataAvailable() override;

  int CheckForNewData() override;

  vtkLidarPosePacketInterpreter* GetPoseInterpreter();
  void SetPoseInterpreter(vtkLidarPosePacketInterpreter* interpreter);

protected:
  vtkLidarPoseStream();
  ~vtkLidarPoseStream() override;

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

private:
  vtkLidarPoseStream(const vtkLidarPoseStream&) = delete;
  void operator=(const vtkLidarPoseStream&) = delete;

  vtkSmartPointer<vtkPolyData> AllPositionsOrientation;

  vtkSmartPointer<vtkTable> AllRawInformation;

  unsigned int LastNumberPositionOrientationInformation;

  unsigned int LastNumberRawInformation;

  int CheckNewDataPositionOrientation();

  int CheckForNewDataRawInformation();

};

#endif
