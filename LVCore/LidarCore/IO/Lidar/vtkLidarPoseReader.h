/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPoseReader.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLidarPoseReader_h
#define vtkLidarPoseReader_h

#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
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

  vtkGetMacro(FileName, std::string);
  void SetFileName(const std::string& filename);

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

  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkMTimeType GetMTime() override;

  vtkGetMacro(PositionOrientationPort, int);
  vtkSetMacro(PositionOrientationPort, int);

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
  void ReadPositionOrientation(vtkSmartPointer<vtkPolyData>& PositionOrientationInfos,
    vtkSmartPointer<vtkTable>& RawInfos);
};

#endif
