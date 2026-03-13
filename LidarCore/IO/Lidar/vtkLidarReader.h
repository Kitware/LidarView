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

#include <vtkEmulatedTimeAlgorithm.h>

#include <memory>

#include "vtkLidarViewDeprecation.h"

#include "lvIOLidarModule.h"

class vtkLidarPacketInterpreter;
class vtkPolyData;

/**
 * @class vtkLidarReader
 *
 * Reads pcap files using vtkLidarPacketInterpreter implementations.
 */
class LVIOLIDAR_EXPORT vtkLidarReader : public vtkEmulatedTimeAlgorithm
{
public:
  static vtkLidarReader* New();
  vtkTypeMacro(vtkLidarReader, vtkEmulatedTimeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * As vtkLidarReader needs to hide its data when the global time is
   * out of its timesteps bound, we need to change behavior of
   * vtkEmulatedTimeAlgorithm::GetNeedsUpdate()
   *
   * @sa RequestUpdateExtent
   */
  bool GetNeedsUpdate(double time) override;

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

  ///@{
  /**
   * Get the lidar output point cloud.
   */
  vtkPolyData* GetOutput();
  vtkPolyData* GetOutput(int);
  ///@}

protected:
  vtkLidarReader();
  ~vtkLidarReader() override;

  /**
   * Open pcap, parse all timesteps and build a frame index.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Change current timesteps to an "out of bound" one when EmptyFrameUpdate is true.
   *
   * @sa GetNeedsUpdate()
   */
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Read pcap data for a specific frame / timestep.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  ///@{
  /**
   * Open/Close the pcap file.
   */
  virtual bool Open();
  bool Open(std::vector<int> ports);
  bool ReadNextPacket(double& timeSinceStart);
  const std::vector<uint8_t>& GetPayload();
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
   * Callback to set reader on modified if interpreter is modified
   */
  void OnInterpreterModifiedEvent();

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
