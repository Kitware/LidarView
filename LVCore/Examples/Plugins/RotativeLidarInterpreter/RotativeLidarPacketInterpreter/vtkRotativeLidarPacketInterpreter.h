/*=========================================================================

  Program:   LidarView
  Module:    vtkRotativeLidarPacketInterpreter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkRotativeLidarPacketInterpreter_h
#define vtkRotativeLidarPacketInterpreter_h

#include <vtkLidarPacketInterpreter.h>

#include <vtkStringArray.h>
#include <vtkTypeInt64Array.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkUnsignedShortArray.h>

#include <memory>

class vtkRotativeLidarPacketInterpreter : public vtkLidarPacketInterpreter
{
public:
  static vtkRotativeLidarPacketInterpreter* New();
  vtkTypeMacro(vtkRotativeLidarPacketInterpreter, vtkLidarPacketInterpreter);

  /**
   * Initializes the lidar calibration or configuration.
   * This method is called during interpreter initialization, whenever the address/port is changed,
   * or when ResetInitializedState() is called.
   */
  void Initialize() override;

  /**
   * Checks if the current packet is valid for the selected lidar model.
   * Return false when invalid. Invalid packets will be skipped.
   */
  bool IsLidarPacket(unsigned char const* data, unsigned int dataLength) override;

  /**
   * Used to builds a frame index map for random access when reading a pcap file.
   * Returns true when a new frame is detected.
   *
   * If available, outLidarDataTime be assigned with the current packet timestamp information.
   * the This method is not called in "stream" mode.
   */
  bool PreProcessPacket(unsigned char const* data,
    unsigned int dataLength,
    double& outLidarDataTime) override;

  /**
   * Processes the packet by extracting and populating point the provided packet.
   * Calls SplitFrame() when necessary.
   *
   * Once SplitFrame() is called, all accumulated points are cleared and output as a new frame.
   */
  void ProcessPacket(unsigned char const* data, unsigned int dataLength) override;

  ///@{
  /**
   * Set/Get the current LiDAR model.
   * If configuration changes are required, the interpreter needs to be uninitialized using
   * ResetInitializedState().
   */
  void SetLidarModel(int type);
  vtkGetMacro(LidarModel, int);
  ///@}

protected:
  /**
   * Creates a new empty frame object, which will be filled by ProcessPacket.
   * Called by the based class, after each frame split.
   */
  vtkSmartPointer<vtkPolyData> CreateNewEmptyFrame(vtkIdType nbrOfPoints,
    vtkIdType prereservedNbrOfPoints = 60000) override;

  vtkRotativeLidarPacketInterpreter();
  ~vtkRotativeLidarPacketInterpreter();

private:
  vtkRotativeLidarPacketInterpreter(const vtkRotativeLidarPacketInterpreter&) = delete;
  void operator=(const vtkRotativeLidarPacketInterpreter&) = delete;

  vtkSmartPointer<vtkPoints> Points;
  vtkSmartPointer<vtkDoubleArray> PointsX;
  vtkSmartPointer<vtkDoubleArray> PointsY;
  vtkSmartPointer<vtkDoubleArray> PointsZ;
  vtkSmartPointer<vtkUnsignedCharArray> Intensity;
  vtkSmartPointer<vtkUnsignedCharArray> LaserId;
  vtkSmartPointer<vtkDoubleArray> Distance;
  vtkSmartPointer<vtkTypeInt64Array> Timestamp;

  unsigned int LastTimestamp = 0;
  unsigned int LastFrameID = 0;
  int LidarModel = 0;
};

#endif // vtkRotativeLidarPacketInterpreter_h
