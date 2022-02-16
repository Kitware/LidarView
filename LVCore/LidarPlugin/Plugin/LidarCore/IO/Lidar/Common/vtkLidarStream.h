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

#include "IO/vtkStream.h"
#include "vtkLidarPacketInterpreter.h"

#include "LidarCoreModule.h"

class LIDARCORE_EXPORT vtkLidarStream : public vtkStream
{
public:
  static vtkLidarStream* New();
  vtkTypeMacro(vtkLidarStream, vtkStream)

  /**
   * @brief GetSensorInformation return some sensor information used for display purposes
   * @param shortVersion True to have a succinct version of the sensor information
   */
  virtual std::string GetSensorInformation(bool shortVersion = false);

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

  vtkGetMacro(DetectFrameDropping, bool)
  vtkSetMacro(DetectFrameDropping, bool)

  void AddNewData() override;

  void ClearAllDataAvailable() override;

  int CheckForNewData() override;

  void Start() override;

  vtkLidarPacketInterpreter* GetLidarInterpreter();
  void SetLidarInterpreter(vtkLidarPacketInterpreter* interpreter);

protected:
  vtkLidarStream();
  ~vtkLidarStream() override;

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * @brief Calibrate Set the Calibration file in the LidarInterpreter and launch calibration
   */
  int Calibrate();

  //! Indicate if we should detect that some frames are dropped
  bool DetectFrameDropping = false;

  //! Last Frame processed, this is important if we want to detect frame dropping
  int LastFrameProcessed = 0;

  //! The calibrationFileName to used and set to the Interpreter once one has been set
  std::string CalibrationFileName = "";

private:
  vtkLidarStream(const vtkLidarStream&) = delete;
  void operator=(const vtkLidarStream&) = delete;

  std::deque<vtkSmartPointer<vtkPolyData> > Frames;
};

#endif // VTKLIDARSTREAM_H
