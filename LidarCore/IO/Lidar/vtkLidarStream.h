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
#include <optional>

#include "vtkLidarPacketInterpreter.h"
#include "vtkStream.h"

#include <vtkPolyData.h>

#include "lvIOLidarModule.h"

class vtkLidarPacketInterpreter;

/**
 * This class implement a base class for all LiDAR stream.
 * It can use different interpreter for different vendors.
 */
class LVIOLIDAR_EXPORT vtkLidarStream : public vtkStream
{
public:
  static vtkLidarStream* New();
  vtkTypeMacro(vtkLidarStream, vtkStream)

  /**
   * Start the LiDAR stream: initialize the packet interpreter and the packet receiver
   */
  void Start() override;

  /**
   * Return some sensor information used for display purposes
   */
  virtual std::string GetSensorInformation(bool shortVersion = false);

  ///@{
  /**
   * If enable a warning will be sent if this class drop frames (processing not fast enough).
   */
  vtkGetMacro(DetectFrameDropping, bool);
  vtkSetMacro(DetectFrameDropping, bool);
  ///@}

  /**
   * Return 1 if there frames in internal queue.
   */
  int CheckForNewData() override;

  ///@{
  /**
   *Set / Get the LiDAR interpreter.
   */
  vtkGetObjectMacro(LidarInterpreter, vtkLidarPacketInterpreter);
  void SetLidarInterpreter(vtkLidarPacketInterpreter* interpreter);
  ///@}

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

  std::optional<double> FrameReceivedTimestamp;
  unsigned long NbOfPacketReceived = 0;
  std::map<unsigned int, unsigned int> PacketReceivedMap;

  unsigned long ReaderObserverId = 0;
  vtkSmartPointer<vtkLidarPacketInterpreter> LidarInterpreter;

  std::deque<vtkSmartPointer<vtkPolyData>> Frames;
};

#endif // VTKLIDARSTREAM_H
