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

  vtkPosePacketInterpreter* GetPoseInterpreter();
  void SetPoseInterpreter(vtkPosePacketInterpreter* interpreter);

  ///@{
  /**
   * Set / Get GNSS port and forward port for the internal stream object.
   */
  int GetGNSSPort();
  void SetGNSSPort(int);
  int GetGNSSForwardedPort();
  void SetGNSSForwardedPort(int);
  ///@}

  ///@{
  /**
   * Overload of stream methods to set both the parent class and
   * the internal stream objects.
   */
  void SetMulticastAddress(const std::string&) override;
  void SetLocalListeningAddress(const std::string&) override;
  void SetForwardedIpAddress(const std::string& ipAddress) override;
  void SetIsForwarding(bool) override;
  void SetIsCrashAnalysing(bool value) override;

  virtual void Start() override;
  virtual void Stop() override;
  vtkMTimeType GetMTime() override;
  ///@}

protected:
  vtkLidarPoseStream();
  ~vtkLidarPoseStream() = default;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkLidarPoseStream(const vtkLidarPoseStream&) = delete;
  void operator=(const vtkLidarPoseStream&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
