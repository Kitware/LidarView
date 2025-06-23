/*=========================================================================

  Program: LidarView
  Module:  vtkLidarStream.h

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTKLIDARSTREAM_H
#define VTKLIDARSTREAM_H

#include <deque>
#include <memory>

#include "vtkLidarPacketInterpreter.h"
#include "vtkStream.h"

#include <vtkPolyData.h>

#include "lvIOLidarModule.h"

class vtkLidarPacketInterpreter;

class LVIOLIDAR_EXPORT vtkLidarStream : public vtkStream
{
public:
  static vtkLidarStream* New();
  vtkTypeMacro(vtkLidarStream, vtkStream)

  void Start() override;

  /**
   * @brief GetSensorInformation return some sensor information used for display purposes
   * @param shortVersion True to have a succinct version of the sensor information
   */
  virtual std::string GetSensorInformation(bool shortVersion = false);

  vtkGetMacro(DetectFrameDropping, bool);
  vtkSetMacro(DetectFrameDropping, bool);

  int CheckForNewData() override;

  vtkGetObjectMacro(LidarInterpreter, vtkLidarPacketInterpreter);
  void SetLidarInterpreter(vtkLidarPacketInterpreter* interpreter);

protected:
  vtkLidarStream();
  ~vtkLidarStream() override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  void ConsumePacket(const std::vector<uint8_t>& pkt, double timestamp) override;

  //! Indicate if we should detect that some frames are dropped
  bool DetectFrameDropping = false;

  //! Last Frame processed, this is important if we want to detect frame dropping
  int LastFrameProcessed = 0;

private:
  vtkLidarStream(const vtkLidarStream&) = delete;
  void operator=(const vtkLidarStream&) = delete;

  /**
   * Callback to set on modified if interpreter is modified
   */
  void OnInterpreterModifiedEvent();

  void AddNewData();

  unsigned long ReaderObserverId = 0;
  vtkSmartPointer<vtkLidarPacketInterpreter> LidarInterpreter;

  std::deque<vtkSmartPointer<vtkPolyData>> Frames;
};

#endif // VTKLIDARSTREAM_H
