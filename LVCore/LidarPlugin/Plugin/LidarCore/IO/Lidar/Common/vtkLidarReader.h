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

#ifndef VTKLIDARREADER_H
#define VTKLIDARREADER_H

#include "vtkLidarPacketInterpreter.h"
#include <vtkPolyDataAlgorithm.h>

#include "LidarCoreModule.h"

class vtkPacketFileReader;

//! @todo a decition should be made if the opening/closing of the pcap should be handle by
//! the class itself of the class user. Currently this is not clear
class LIDARCORE_EXPORT vtkLidarReader : public vtkPolyDataAlgorithm
{
public:
  static vtkLidarReader* New();
  vtkTypeMacro(vtkLidarReader, vtkPolyDataAlgorithm)

  int GetNumberOfFrames() { return this->FrameCatalog.size(); }

  /**
   * @copydoc FileName
   */
  vtkGetMacro(FileName, std::string)
  virtual void SetFileName(const std::string& filename);

  vtkGetMacro(DetectFrameDropping, bool)
  vtkSetMacro(DetectFrameDropping, bool)

  vtkMTimeType GetMTime() override;

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

  /**
   * @copydoc vtkLidarPacketInterpreter
   */
  vtkGetObjectMacro(Interpreter, vtkLidarPacketInterpreter)
  vtkSetObjectMacro(Interpreter, vtkLidarPacketInterpreter)

  /**
   * @copydoc NetworkTimeToDataTime
   */
  vtkGetMacro(NetworkTimeToDataTime, double)

  /**
   * @brief GetFrame returns the requested frame
   * @param frameNumber beteween 0 and vtkLidarReader::GetNumberOfFrames()
   */
  virtual vtkSmartPointer<vtkPolyData> GetFrame(int frameNumber);

  /**
   * @brief GetFrameForPacketTime returns the requested frame
   * @param packetTime udp packet time requested
   */
  virtual vtkSmartPointer<vtkPolyData> GetFrameForPacketTime(double packetTime);

  /**
   * @brief GetFrameForDataTime returns the requested frame
   * @param dataTime data time requested
   */
  virtual vtkSmartPointer<vtkPolyData> GetFrameForDataTime(double dataTime);

  /**
   * @brief GetFrameIndexForPacketTime returns the frame index
   *        corresponding to the packet time asked
   * @param packetTime udp packet time requested
   */
  virtual int GetFrameIndexForPacketTime(double packetTime);

  /**
   * @brief GetFrameIndexForDataTime returns the frame index
   *        corresponding to the packet time asked
   * @param packetTime udp packet time requested
   */
  virtual int GetFrameIndexForDataTime(double dataTime);

  /**
   * @brief Open open the pcap file
   * @todo a decition should be made if the opening/closing of the pcap should be handle by
   * the class itself of the class user. Currently this is not clear
   * @param reassemble Controls vtkPacketFileReader IP Reassembly
   */
  virtual void Open(bool reassemble = true);

  /**
   * @brief Close close the pcap file
   * @todo a decition should be made if the opening/closing of the pcap should be handle by
   * the class itself of the class user. Currently this is not clear
   */
  virtual void Close();

  /**
   * @brief SaveFrame save the packet corresponding to the desired frames in a pcap file.
   * Because we are saving network packet, part of previous and/or next frames could be included in generated the pcap
   * @param startFrame first frame to record
   * @param endFrame last frame to record, this frame is included
   * @param filename where to save the generate pcap file
   */
  virtual void SaveFrame(int startFrame, int endFrame, const std::string& filename);

  vtkGetMacro(ShowFirstAndLastFrame, bool)
  vtkSetMacro(ShowFirstAndLastFrame, bool)

  vtkGetMacro(UsePacketTimeForDisplayTime, bool)
  vtkSetMacro(UsePacketTimeForDisplayTime, bool)

  int GetLidarPort() { return this->LidarPort; }
  void SetLidarPort(int _arg);

protected:
  vtkLidarReader();
  ~vtkLidarReader() = default;

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  //! Indicate if we should detect that some frames are dropped
  bool DetectFrameDropping = false;

  //! Last Frame processed, this is important if we want to detect frame dropping
  int LastFrameProcessed = 0;

  //! The calibrationFileName to used and set to the Interpreter once one has been set
  std::string CalibrationFileName = "";

  //! Name of the pcap file to read
  std::string FileName = "";

  //! Interpret the packet to create a frame, all the magic happen here
  vtkLidarPacketInterpreter* Interpreter = nullptr;

  //! Miscellaneous information about a frame that enable:
  //! - Quick jump to a frame index
  //! - Computation of an adjustedtimestamp
  //! - ...
  std::vector<FrameInformation> FrameCatalog;

  //! Show/Hide the first and last frame that most of the time are partial frames
  bool ShowFirstAndLastFrame = false;

  //! libpcap wrapped reader which enable to get the raw pcap packet from the pcap file
  vtkPacketFileReader* Reader = nullptr;

  //! Filter the packet to only read the packet received on a specify port
  //! To read all packet use -1
  int LidarPort = -1;

  //! True to display the packet time in the UI pipeline
  //! False to display the network time in the UI pipeline
  bool UsePacketTimeForDisplayTime = false;

private:
  /**
   * @brief ReadFrameInformation read the whole pcap and create a frame index.
   * In case the calibration is contained in the pcap file, this will also read it
   */
  int ReadFrameInformation();
  /**
   * @brief SetTimestepInformation Set the timestep available
   * @param info
   */
  void SetTimestepInformation(vtkInformation *info);

  vtkLidarReader(const vtkLidarReader&) = delete;
  void operator=(const vtkLidarReader&) = delete;

  /**
   * @brief The timeshift between these two times.
   * When added to "network time" it gives "data time".
   * Example of "network time": the pipeline time (in general)
   * Example of "data time": the time of the lidar points.
   */
  double NetworkTimeToDataTime = 0.0;
};

#endif // VTKLIDARREADER_H
