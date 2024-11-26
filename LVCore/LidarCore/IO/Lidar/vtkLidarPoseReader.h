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

#include "vtkLidarReader.h"
#include "vtkLidarViewDeprecation.h"
#include "vtkPosePacketInterpreter.h"

#include "lvIOLidarModule.h"

class vtkPosePacketInterpreter;

/**
 * The vtkLidarPoseReader reads both LiDAR data and pose data
 * from a .pcap file.
 *
 * It's important to note that the pose data is aggregated (non-temporal) in
 * comparison to the temporal LiDAR data.
 */
class LVIOLIDAR_EXPORT vtkLidarPoseReader : public vtkLidarReader
{
public:
  static vtkLidarPoseReader* New();
  vtkTypeMacro(vtkLidarPoseReader, vtkLidarReader)

  vtkGetObjectMacro(PoseInterpreter, vtkPosePacketInterpreter)
  vtkSetObjectMacro(PoseInterpreter, vtkPosePacketInterpreter)

  /**
   * Call vtkLidarReader::Open with the addition of the pose port.
   */
  bool Open() override;
  LIDARVIEW_DEPRECATED_IN_5_1_0("Please use Open() methods without reassemble instead")
  bool Open(bool reassemble) override;

  vtkMTimeType GetMTime() override;

  vtkGetMacro(LidarPosePort, int);
  vtkSetMacro(LidarPosePort, int);

protected:
  vtkLidarPoseReader();
  ~vtkLidarPoseReader() = default;

  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  //! Interpret for pose packet
  vtkPosePacketInterpreter* PoseInterpreter = nullptr;

  //! Filter the packet to only read the packet received on a specify port
  //! To read all packet use -1
  int LidarPosePort = -1;

private:
  vtkLidarPoseReader(const vtkLidarPoseReader&) = delete;
  void operator=(const vtkLidarPoseReader&) = delete;

  ///@{
  /**
   * Pose data are only read when Apply is called
   * compared to LiDAR data which are read each time a new frame is requested.
   */
  bool ReadLidarPoseData = true;
  vtkSmartPointer<vtkPolyData> PoseInfo;
  vtkSmartPointer<vtkTable> RawInfos;
  ///@}

  /**
   * Returns all the positions contained in the pcap file
   */
  void ReadPoses();
};

#endif
