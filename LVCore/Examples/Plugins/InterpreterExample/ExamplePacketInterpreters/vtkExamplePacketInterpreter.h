/*=========================================================================

  Program:   LidarView
  Module:    vtkExamplePacketInterpreter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkExamplePacketInterpreter_h
#define vtkExamplePacketInterpreter_h

#include <vtkLidarPacketInterpreter.h>

#include <vtkStringArray.h>
#include <vtkTypeInt64Array.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkUnsignedShortArray.h>

#include <memory>

#include "ExamplePacketInterpretersModule.h"

class EXAMPLEPACKETINTERPRETERS_EXPORT vtkExamplePacketInterpreter
  : public vtkLidarPacketInterpreter
{
public:
  static vtkExamplePacketInterpreter* New();
  vtkTypeMacro(vtkExamplePacketInterpreter, vtkLidarPacketInterpreter);
  void PrintSelf(ostream& vtkNotUsed(os), vtkIndent vtkNotUsed(indent)) override{};

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
   * Builds a frame index for random access when reading a pcap file.
   * Returns true when a new frame is detected.
   *
   * You should assigns time step information from the lidar packet to the outLidarDataTime
   * variable, if available.
   */
  bool PreProcessPacket(unsigned char const* data,
    unsigned int dataLength,
    double& outLidarDataTime) override;

  /**
   * Processes the packet, filling point information using packet data,
   * and calling SplitFrame when necessary.
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
   */
  vtkSmartPointer<vtkPolyData> CreateNewEmptyFrame(vtkIdType nbrOfPoints,
    vtkIdType prereservedNbrOfPoints = 60000) override;

  vtkExamplePacketInterpreter();
  ~vtkExamplePacketInterpreter();

private:
  vtkExamplePacketInterpreter(const vtkExamplePacketInterpreter&) = delete;
  void operator=(const vtkExamplePacketInterpreter&) = delete;

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

#endif // vtkExamplePacketInterpreter_h
