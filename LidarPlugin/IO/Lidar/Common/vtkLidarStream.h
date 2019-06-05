// Copyright 2013 Velodyne Acoustics, Inc.
// Copyright 2018 Kitware SAS.
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

#ifndef VTKLIDARSTREAM_H
#define VTKLIDARSTREAM_H

#include <memory>
#include "vtkLidarProvider.h"

class PacketConsumer;
class PacketFileWriter;
class NetworkSource;

class VTK_EXPORT vtkLidarStream : public vtkLidarProvider
{
public:
  static vtkLidarStream* New();
  vtkTypeMacro(vtkLidarStream, vtkLidarProvider)

  int GetNumberOfFrames() override;

  void Start();
  void Stop();

  /**
   * @copydoc vtkLidarStreamInternal::OutputFileName
   */
  std::string GetOutputFile();
  void SetOutputFile(const std::string& filename);

  /**
   * @copydoc NetworkSource::LidarPort
   */
  void SetLidarPort(int) override;
  int GetLidarPort() override;

  /**
   * @copydoc NetworkSource::GPSPort
   */
  int GetGPSPort();
  void SetGPSPort(int);

  /**
   * @copydoc NetworkSource::ForwardedIpAddress
   */
  std::string GetForwardedIpAddress();
  void SetForwardedIpAddress(const std::string& ipAddress);

  /**
   * @copydoc NetworkSource::ForwardedLidarPort
   */
  int GetForwardedLidarPort();
  void SetForwardedLidarPort(int);

  /**
   * @copydoc NetworkSource::ForwardedLidarPort
   */
  int GetForwardedGPSPort();
  void SetForwardedGPSPort(int);

  void EnableGPSListening(bool);

  /**
   * @copydoc NetworkSource::IsForwarding
   */
  bool GetIsForwarding();
  void SetIsForwarding(bool);

  /**
   * @copydoc NetworkSource::IsCrashAnalysing
   */
  bool GetIsCrashAnalysing();
  void SetIsCrashAnalysing(bool value);

  /**
   * @brief GetNeedsUpdate
   * @return true if a new frame is ready
   */
  bool GetNeedsUpdate();

protected:
  vtkLidarStream();
  ~vtkLidarStream();

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

  //! where to save a live record of the sensor
  std::string OutputFileName = "";
  std::shared_ptr<PacketConsumer> Consumer;
  std::shared_ptr<PacketFileWriter> Writer;
  std::unique_ptr<NetworkSource> Network;
private:
  vtkLidarStream(const vtkLidarStream&) = delete;
  void operator=(const vtkLidarStream&) = delete;
};

#endif // VTKLIDARSTREAM_H
