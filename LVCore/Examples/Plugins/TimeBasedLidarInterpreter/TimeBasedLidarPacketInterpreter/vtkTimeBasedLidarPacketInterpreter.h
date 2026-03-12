/*=========================================================================

  Program:   LidarView
  Module:    vtkTimeBasedLidarPacketInterpreter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkTimeBasedLidarPacketInterpreter_h
#define vtkTimeBasedLidarPacketInterpreter_h

#include <vtkLidarPacketInterpreter.h>

#include <vtkFloatArray.h>
#include <vtkSmartPointer.h>
#include <vtkTypeInt64Array.h>

#include <memory>

class vtkTimeBasedLidarPacketInterpreter : public vtkLidarPacketInterpreter
{
public:
  static vtkTimeBasedLidarPacketInterpreter* New();
  vtkTypeMacro(vtkTimeBasedLidarPacketInterpreter, vtkLidarPacketInterpreter);

  /**
   * Initializes the lidar calibration or configuration.
   * This method is called during interpreter initialization, whenever the address / port /
   * publish interval is changed, or when ResetInitializedState() is called.
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
   * Calls SplitFrame() when necessary, using DoSplitPacket().
   *
   * Once SplitFrame() is called, all accumulated points are cleared and output as a new frame.
   */
  void ProcessPacket(unsigned char const* data, unsigned int dataLength) override;

  ///@{
  /**
   * Interval representing a "time length" of a frame (in ms).
   *
   * Note: vtkSetAndResetInitializeMacro is used here as we would need to rebuild
   * the frame index (in .pcap mode), it automatically call ResetInitializedState().
   */
  vtkGetMacro(PublishInterval, long);
  vtkSetAndResetInitializeMacro(PublishInterval, long);
  ///@}

protected:
  /**
   * Creates a new empty frame object, which will be filled by ProcessPacket.
   * Called by the based class, after each frame split.
   */
  vtkSmartPointer<vtkPolyData> CreateNewEmptyFrame(vtkIdType nbrOfPoints,
    vtkIdType prereservedNbrOfPoints = 60000) override;

  vtkTimeBasedLidarPacketInterpreter();
  ~vtkTimeBasedLidarPacketInterpreter();

private:
  vtkTimeBasedLidarPacketInterpreter(const vtkTimeBasedLidarPacketInterpreter&) = delete;
  void operator=(const vtkTimeBasedLidarPacketInterpreter&) = delete;

  /**
   * Checks if the time difference between the last frame timestamp and the current
   * packet timestamp meets or exceeds the specified publish interval.
   * Returns true if enough time has passed to split the frame.
   */
  bool DoSplitPacket(double timestamp);

  double LastTimestamp;
  long PublishInterval = 100; // in ms

  vtkSmartPointer<vtkPoints> Points;
  vtkSmartPointer<vtkFloatArray> X;
  vtkSmartPointer<vtkFloatArray> Y;
  vtkSmartPointer<vtkFloatArray> Z;
  vtkSmartPointer<vtkFloatArray> Reflectance;
  vtkSmartPointer<vtkTypeInt64Array> Timestamp;
};

#endif // vtkTimeBasedLidarPacketInterpreter_h
