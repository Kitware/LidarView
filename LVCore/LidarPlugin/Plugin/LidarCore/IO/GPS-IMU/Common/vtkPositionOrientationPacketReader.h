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

#ifndef VTKPOSITIONORIENTATIONPACKETREADER_H
#define VTKPOSITIONORIENTATIONPACKETREADER_H

#include <vtkPolyDataAlgorithm.h>
#include <vtkPacketFileReader.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include "vtkPositionOrientationPacketInterpreter.h"

#include "LidarCoreModule.h"

class LIDARCORE_EXPORT vtkPositionOrientationPacketReader : public vtkPolyDataAlgorithm
{
public:
  static vtkPositionOrientationPacketReader* New();
  vtkTypeMacro(vtkPositionOrientationPacketReader, vtkPolyDataAlgorithm)

  vtkGetMacro(FileName, std::string)
  void SetFileName(const std::string &filename);

  vtkGetObjectMacro(Interpreter, vtkPositionOrientationPacketInterpreter)
  vtkSetObjectMacro(Interpreter, vtkPositionOrientationPacketInterpreter)

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
  vtkPositionOrientationPacketReader();
  ~vtkPositionOrientationPacketReader() = default;

  //! Interpret for position orientation packet
  vtkPositionOrientationPacketInterpreter* Interpreter = nullptr;

  //! Name of the pcap file to read
  std::string FileName = "";

  //! libpcap wrapped reader which enable to get the raw pcap packet from the pcap file
  vtkPacketFileReader* Reader = nullptr;

  //! Filter the packet to only read the packet received on a specify port
  //! To read all packet use -1
  int PositionOrientationPort = -1;

private:
  vtkPositionOrientationPacketReader(const vtkPositionOrientationPacketReader&) = delete;
  void operator=(const vtkPositionOrientationPacketReader&) = delete;
  /**
   * @brief ReadPositionOrientation returns all the positions contains in the pcap file
   */
  void ReadPositionOrientation(vtkSmartPointer<vtkPolyData> & PositionOrientationInfos,
                               vtkSmartPointer<vtkTable> & RawInfos);
};

#endif // VTKPOSITIONORIENTATIONPACKETREADER_H
