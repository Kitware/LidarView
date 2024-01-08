/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPoseStream.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLidarPoseStream_h
#define vtkLidarPoseStream_h

#include <memory>

#include <vtkPolyData.h>
#include <vtkTable.h>

#include "vtkLidarPosePacketInterpreter.h"
#include "vtkStream.h"

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
