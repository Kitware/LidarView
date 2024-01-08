//=========================================================================
// Copyright 2020 Kitware, Inc.
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
//=========================================================================

#ifndef vtkLidarPoseReader_h
#define vtkLidarPoseReader_h

#include <vtkPolyDataAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include "vtkLidarPosePacketInterpreter.h"
#include "vtkPacketFileReader.h"

#include "lvIOLidarModule.h"

class LVIOLIDAR_EXPORT vtkLidarPoseReader : public vtkPolyDataAlgorithm
{
public:
  static vtkLidarPoseReader* New();
  vtkTypeMacro(vtkLidarPoseReader, vtkPolyDataAlgorithm)

  vtkGetMacro(FileName, std::string)
  void SetFileName(const std::string &filename);

  vtkGetObjectMacro(Interpreter, vtkLidarPosePacketInterpreter)
  vtkSetObjectMacro(Interpreter, vtkLidarPosePacketInterpreter)

  /**
   * @brief Open open the pcap file
   */
  void Open();

  /**
   * @brief Close close the pcap file
   */
  void Close();

  int RequestData(vtkInformation *vtkNotUsed(request),
                  vtkInformationVector **vtkNotUsed(inputVector),
                  vtkInformationVector *outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkMTimeType GetMTime() override;

  vtkGetMacro(PositionOrientationPort, int)
  vtkSetMacro(PositionOrientationPort, int)

protected:
  vtkLidarPoseReader();
  ~vtkLidarPoseReader() = default;

  //! Interpret for position orientation packet
  vtkLidarPosePacketInterpreter* Interpreter = nullptr;

  //! Name of the pcap file to read
  std::string FileName = "";

  //! libpcap wrapped reader which enable to get the raw pcap packet from the pcap file
  vtkPacketFileReader* Reader = nullptr;

  //! Filter the packet to only read the packet received on a specify port
  //! To read all packet use -1
  int PositionOrientationPort = -1;

private:
  vtkLidarPoseReader(const vtkLidarPoseReader&) = delete;
  void operator=(const vtkLidarPoseReader&) = delete;
  /**
   * @brief ReadPositionOrientation returns all the positions contains in the pcap file
   */
  void ReadPositionOrientation(vtkSmartPointer<vtkPolyData> & PositionOrientationInfos,
                               vtkSmartPointer<vtkTable> & RawInfos);
};

#endif
