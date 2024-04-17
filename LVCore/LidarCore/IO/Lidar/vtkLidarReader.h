/*=========================================================================

  Program: LidarView
  Module:  vtkLidarReader.h

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLidarReader_h
#define vtkLidarReader_h

#include <vtkPolyDataAlgorithm.h>

#include <memory>

#include "lvIOLidarModule.h"

class vtkLidarPacketInterpreter;
class vtkPolyData;

/**
 * @class vtkLidarReader
 *
 * Reads pcap files using vtkLidarPacketInterpreter implementations.
 */
class LVIOLIDAR_EXPORT vtkLidarReader : public vtkPolyDataAlgorithm
{
public:
  static vtkLidarReader* New();
  vtkTypeMacro(vtkLidarReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override MTime with interpreter one. So when the interpreter is updated
   * the LidarReader is also considered updated.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get pcap filename
   */
  vtkGetMacro(FileName, std::string);
  void SetFileName(const std::string& filename);
  ///@}

  ///@{
  /**
   * Set/Get lidar port.
   * Filter the packet to only read the packet received on a specify port.
   * To read all packet use -1. Default to -1
   */
  vtkGetMacro(LidarPort, int);
  void SetLidarPort(int port);
  ///@}

  ///@{
  /**
   * When enabled, warn user about frames dropped.
   * Default to false.
   */
  vtkGetMacro(DetectFrameDropping, bool);
  vtkSetMacro(DetectFrameDropping, bool);
  ///@}

  ///@{
  /**
   * When enabled, show partial frames, such as the first and last frames.
   * It is common that theses frame are incomplete.
   * Default to false.
   */
  vtkGetMacro(ShowPartialFrames, bool);
  void SetShowPartialFrames(bool show);
  ///@}

  ///@{
  /**
   * Set/Get the time type given to vtk pipeline and therefore displayed in UI.
   * - USE_NETWORK_TIME: Uses time available in network packets (unrelated to packet content)
   * - USE_LIDAR_TIME: Uses time given by LiDAR interpreter (e.g seconds since LiDAR started)
   *
   * Default to USE_NETWORK_TIME.
   */
  enum TimeType
  {
    USE_NETWORK_TIME = 0,
    USE_LIDAR_TIME,

    Size
  };
  vtkSetClampMacro(DisplayTimeType, int, 0, TimeType::Size);
  vtkGetMacro(DisplayTimeType, int);
  ///@}

  ///@{
  /**
   * Set/Get the interpreter, which should be inheriting from base class vtkLidarPacketInterpreter.
   * It is then used to parse the pcap file and create a frame catalog.
   */
  vtkGetObjectMacro(LidarInterpreter, vtkLidarPacketInterpreter);
  void SetLidarInterpreter(vtkLidarPacketInterpreter*);
  ///@}

  /**
   * Save the packet corresponding to the desired frames in a pcap file.
   * Because we are saving network packet, part of previous and/or next frames could be included in
   * generated the pcap.
   */
  void SaveFrames(unsigned int startFrame, unsigned int endFrame, const std::string& filename);

  /**
   * Compute the median difference on all frames between network time,
   * and the LiDAR data time (first lidar point time).
   */
  double GetNetworkTimeToDataTime();

  /**
   * Return sensor information given by the underlying interpreter.
   */
  std::string GetSensorInformation(bool shortVersion = false);

protected:
  vtkLidarReader();
  ~vtkLidarReader() override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  ///@{
  /**
   * Open/Close the pcap file.
   */
  virtual bool Open(bool reassemble = true);
  bool Open(std::vector<int> ports, bool reassemble = true);
  bool ReadNextPacket(const unsigned char*& data, unsigned int& dataLength, double& timeSinceStart);
  void Close();
  ///@}

  /**
   * Reset the indexing of pcap frames on the next request information.
   * (May add significant time overhead depending of pcap size)
   */
  void ResetFrameIndexes();

private:
  vtkLidarReader(const vtkLidarReader&) = delete;
  void operator=(const vtkLidarReader&) = delete;

  /**
   * Read the whole pcap to create frame index.
   */
  bool BuildFramesIndex();

  /**
   * Read the pcap frame for a specific index.
   */
  bool ReadFrame(size_t index, vtkPolyData* output);

  std::string FileName;
  int LidarPort = -1;
  bool DetectFrameDropping = false;
  bool ShowPartialFrames = false;
  int DisplayTimeType = USE_NETWORK_TIME;
  vtkLidarPacketInterpreter* LidarInterpreter = nullptr;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
