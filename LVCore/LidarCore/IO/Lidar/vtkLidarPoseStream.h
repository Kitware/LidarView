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

#include "vtkLidarStream.h"
#include "vtkPosePacketInterpreter.h"

#include "lvIOLidarModule.h"

class vtkPosePacketInterpreter;

/**
 * vtkLidarPoseStream is composed by two different streams:
 *  - The standard Lidar stream containing LiDAR data.
 *  - Position and orientation data (GNSS + IMU), which are aggregated
 *    into another output.
 *
 * The current class structure is suboptimal, as it opens two separate
 * streams for the same host but on different ports.
 */
class LVIOLIDAR_EXPORT vtkLidarPoseStream : public vtkLidarStream
{
public:
  static vtkLidarPoseStream* New();
  vtkTypeMacro(vtkLidarPoseStream, vtkLidarStream)

  vtkMTimeType GetMTime() override;

  void Start() override;

  vtkGetObjectMacro(PoseInterpreter, vtkPosePacketInterpreter);
  void SetPoseInterpreter(vtkPosePacketInterpreter* interpreter);

  ///@{
  /**
   * Set / Get GNSS port for the internal stream object.
   */
  vtkGetMacro(GNSSPort, unsigned int);
  vtkSetMacro(GNSSPort, unsigned int);
  LIDARVIEW_DEPRECATED_IN_5_1_0("Please use vtkUDPPacketReceiver methods instead")
  void SetGNSSForwardedPort(int);
  LIDARVIEW_DEPRECATED_IN_5_1_0("Please use vtkUDPPacketReceiver methods instead")
  int GetGNSSForwardedPort();
  ///@}

  void AddNewData() override;

  void ClearAllDataAvailable() override;

  int CheckForNewData() override;

protected:
  vtkLidarPoseStream();
  ~vtkLidarPoseStream() = default;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  void ConsumePacket(const std::vector<uint8_t>& pkt, double timestamp) override;

private:
  vtkLidarPoseStream(const vtkLidarPoseStream&) = delete;
  void operator=(const vtkLidarPoseStream&) = delete;

  /*!< The port to receive information*/
  unsigned int GNSSPort = 8308;

  vtkSmartPointer<vtkPosePacketInterpreter> PoseInterpreter;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
