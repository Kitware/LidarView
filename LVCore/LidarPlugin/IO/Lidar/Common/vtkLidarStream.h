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

#include <deque>
#include <memory>

#include "vtkStream.h"
#include "vtkLidarPacketInterpreter.h"

class VTK_EXPORT vtkLidarStream : public vtkStream
{
public:
  static vtkLidarStream* New();
  vtkTypeMacro(vtkLidarStream, vtkStream)

  int GetNumberOfFrames();

  /**
   * @brief GetSensorInformation return some sensor information used for display purposes
   */
  virtual std::string GetSensorInformation();

  /**
   * @copydoc vtkLidarPacketInterpreter::CalibrationFileName
   */
  virtual void SetCalibrationFileName(const std::string& filename);

  /**
   * @brief SetDummyProperty a trick to workaround failure to wrap LaserSelection, this actually only calls Modified,
   * however for some obscure reason, doing the same from python does not have the same effect
   * @todo set how to remove this methode as it is a workaround
   */
  void SetDummyProperty(int);

  /**
   * @copydoc vtkLidarPacketInterpreter
   */
  vtkGetObjectMacro(LidarInterpreter, vtkLidarPacketInterpreter)
  vtkSetObjectMacro(LidarInterpreter, vtkLidarPacketInterpreter)

  vtkGetMacro(DetectFrameDropping, bool)
  vtkSetMacro(DetectFrameDropping, bool)

  vtkMTimeType GetMTime() override;

  void AddNewData() override;

  void ClearAllDataAvailable() override;

  int CheckForNewData() override;

  vtkSmartPointer<vtkInterpreter> GetInterpreter() override;

  void Start() override;

  // Old fonction of vtkLidarStream : deprecated
  // Replace by non lidar/gps specific function because all stream are separated
  // Still call in some client specific files
  [[deprecated("Lidar and Gps input each have their network : use SetListeningPort instead")]]
  void SetLidarPort(int port){vtkStream::SetListeningPort(port);}
  [[deprecated("Lidar and Gps input each have their network : use GetListeningPort instead")]]
  int GetLidarPort(){return vtkStream::GetListeningPort();}
  [[deprecated("Lidar and Gps input each have their network : use SetListeningPort instead")]]
  void SetGPSPort(int port){vtkStream::SetListeningPort(port);}
  [[deprecated("Lidar and Gps input each have their network : use GetListeningPort instead")]]
  int GetGPSPort(){return vtkStream::GetListeningPort();}
  [[deprecated("Lidar and Gps input each have their network : use SetForwardedPort instead")]]
  void SetForwardedLidarPort(int port){vtkStream::SetForwardedPort(port);}
  [[deprecated("Lidar and Gps input each have their network : use GetForwardedPort instead")]]
  int GetForwardedLidarPort(){return vtkStream::GetForwardedPort();}
  [[deprecated("Lidar and Gps input each have their network : use SetForwardedPort instead")]]
  void SetForwardedGPSPort(int port){vtkStream::SetForwardedPort(port);}
  [[deprecated("Lidar and Gps input each have their network : use GetForwardedPort instead")]]
  int GetForwardedGPSPort(){return vtkStream::GetForwardedPort();}
  [[deprecated("Lidar and Gps input each have their network : do nothing anymore,  can be removed")]]
  void EnableGPSListening(bool vtkNotUsed(enable)){/* Nothing to do anymore*/}
  // Some of specific client file (applogic) accessed directly to some removed attribute
  // For example in "openSensor" :
  // sensor.LidarPort = LidarPort should be sensor.GetClientSideObject().SetLidarPort(LidarPort)

protected:
  vtkLidarStream();
  ~vtkLidarStream() = default;

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info);

  /**
   * @brief Calibrate Set the Calibration file in the LidarInterpreter and launch calibration
   */
  int Calibrate();

  //! Indicate if we should detect that some frames are dropped
  bool DetectFrameDropping = false;

  //! Last Frame processed, this is important if we want to detect frame dropping
  int LastFrameProcessed = 0;

  //! Interpret the packet to create a frame, all the magic happen here
  vtkSmartPointer<vtkLidarPacketInterpreter> LidarInterpreter = nullptr;

  //! The calibrationFileName to used and set to the Interpreter once one has been set
  std::string CalibrationFileName = "";

private:
  vtkLidarStream(const vtkLidarStream&) = delete;
  void operator=(const vtkLidarStream&) = delete;

  std::deque<vtkSmartPointer<vtkPolyData> > Frames;
};

#endif // VTKLIDARSTREAM_H
