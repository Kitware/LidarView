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

// This needs to be included first because vtkSystemIncludes.h is included
// instead of stdio.h so the CMake variable __USE_LARGEFILE64 must be consistent.
// Otherwise fpos_t has 2 differents definition _G_fpos_t and _G_fpos64_t
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

  void ProcessPacket(unsigned char const* data, unsigned int dataLength) override;

  bool IsLidarPacket(unsigned char const* data, unsigned int dataLength) override;

  bool PreProcessPacket(unsigned char const* data,
    unsigned int dataLength,
    fpos_t filePosition = fpos_t(),
    double packetNetworkTime = 0,
    std::vector<FrameInformation>* frameCatalog = nullptr) override;

  std::string GetSensorInformation(bool shortVersion = false) override;

  std::string GetSensorName();

  void LoadCalibration(const std::string& filename) override;

protected:
  vtkSmartPointer<vtkPolyData> CreateNewEmptyFrame(vtkIdType nbrOfPoints,
    vtkIdType prereservedNbrOfPoints = 60000) override;

  vtkSmartPointer<vtkPoints> Points;
  vtkSmartPointer<vtkDoubleArray> PointsX;
  vtkSmartPointer<vtkDoubleArray> PointsY;
  vtkSmartPointer<vtkDoubleArray> PointsZ;
  vtkSmartPointer<vtkUnsignedCharArray> Intensity;
  vtkSmartPointer<vtkUnsignedCharArray> LaserId;
  vtkSmartPointer<vtkDoubleArray> Distance;
  vtkSmartPointer<vtkTypeInt64Array> Timestamp;

  unsigned int LastTimestamp = 0;

  vtkExamplePacketInterpreter();
  ~vtkExamplePacketInterpreter() = default;

private:
  vtkExamplePacketInterpreter(const vtkExamplePacketInterpreter&) = delete;
  void operator=(const vtkExamplePacketInterpreter&) = delete;
};

struct ExampleSpecificFrameInformation : public SpecificFrameInformation
{
  // Keep the frame id
  unsigned int frameID = 0;

  void reset() override { *this = ExampleSpecificFrameInformation(); }
  std::unique_ptr<SpecificFrameInformation> clone() override
  {
    return std::make_unique<ExampleSpecificFrameInformation>(*this);
  }
};

#endif // vtkExamplePacketInterpreter_h
