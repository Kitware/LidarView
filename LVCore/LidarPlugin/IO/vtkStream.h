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

#ifndef VTKSTREAM_H
#define VTKSTREAM_H

#include <memory>
#include <string>

#include <vtkDataObjectAlgorithm.h>
#include <vtkSmartPointer.h>

#include "vtkInterpreter.h"

class PacketConsumer;
class PacketFileWriter;
class NetworkSource;

class VTK_EXPORT vtkStream : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkStream, vtkDataObjectAlgorithm)

  virtual void Start();
  void Stop();

  std::string GetOutputFile();
  void SetOutputFile(const std::string& filename);

  /**
   * @copydoc NetworkSource::LidarPort
   */
  int GetListeningPort();
  void SetListeningPort(int);

  /**
   * @brief multicast Address
   */
  std::string GetMulticastAddress();
  void SetMulticastAddress(const std::string&);

  /**
   * @brief Local Listening Address
   */
  std::string GetLocalListeningAddress();
  void SetLocalListeningAddress(const std::string&);

  /**
   * @copydoc NetworkSource::ForwardedIpAddress
   */
  std::string GetForwardedIpAddress();
  void SetForwardedIpAddress(const std::string& ipAddress);

  /**
   * @copydoc NetworkSource::ForwardedPort
   */
  int GetForwardedPort();
  void SetForwardedPort(int);

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
   * @return true if a new data is ready
   */
  bool GetNeedsUpdate();

  /**
   * @brief AddNewData Add the new data available to the specific buffer of the Stream
   */
  virtual void AddNewData() = 0;

  /**
   * @brief ClearAllDataAvailable Clear the buffer that contains data
   */
  virtual void ClearAllDataAvailable() = 0;

  /**
   * @brief CheckForNewData Check if there is new data available
   */
  virtual int CheckForNewData() = 0;

  /**
   * @brief GetInterpreter Get specific subclass interpreter
   */
  virtual vtkSmartPointer<vtkInterpreter> GetInterpreter() = 0;

protected:
  vtkStream();
  ~vtkStream();

  virtual int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) = 0;

  //! where to save a live record of the sensor
  std::string OutputFileName = "";
  std::shared_ptr<PacketConsumer> Consumer;
  std::shared_ptr<PacketFileWriter> Writer;
  std::unique_ptr<NetworkSource> Network;

private:
  vtkStream(const vtkStream&) = delete;
  void operator=(const vtkStream&) = delete;

  template<class T> void SetAttributeAndRestartIfRunning(T& attribute, const T& value);
};

#endif // VTKSTREAM_H
