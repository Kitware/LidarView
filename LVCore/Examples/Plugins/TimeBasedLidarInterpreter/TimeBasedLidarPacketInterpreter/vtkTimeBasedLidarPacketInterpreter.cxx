/*=========================================================================

  Program:   LidarView
  Module:    vtkTimeBasedLidarPacketInterpreter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTimeBasedLidarPacketInterpreter.h"
#include "InterpreterHelper.h"

#include "TimeBasedLidarFormat.h"

#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

namespace
{
constexpr double US_TO_MS_FACTOR = 1e-3;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTimeBasedLidarPacketInterpreter)

//-----------------------------------------------------------------------------
vtkTimeBasedLidarPacketInterpreter::vtkTimeBasedLidarPacketInterpreter()
{
  // Theses information will be stored in point cloud field data
  this->SetSensorVendor("Kitware");
  this->SetSensorModelName("TimeBasedLidarExample");

  this->ResetCurrentFrame();
}

//-----------------------------------------------------------------------------
vtkTimeBasedLidarPacketInterpreter::~vtkTimeBasedLidarPacketInterpreter() = default;

//-----------------------------------------------------------------------------
void vtkTimeBasedLidarPacketInterpreter::ProcessPacket(unsigned char const* data,
  unsigned int dataLength)
{
  if (!this->IsLidarPacket(data, dataLength))
  {
    return;
  }

  const PacketType* dataPacket = reinterpret_cast<const PacketType*>(data);

  // Detection of a new frame
  if (this->DoSplitPacket(dataPacket->data[0].timestamp * ::US_TO_MS_FACTOR))
  {
    this->SplitFrame();
  }

  for (unsigned int idx = 0; idx < NB_OF_POINTS; idx++)
  {
    const PointType& point = dataPacket->data[idx];
    double pos[3] = { point.x, point.y, point.z };

    // Assign points value to vtk data structure
    this->Points->InsertNextPoint(pos);
    InsertNextValueIfNotNull(this->X, pos[0]);
    InsertNextValueIfNotNull(this->Y, pos[1]);
    InsertNextValueIfNotNull(this->Z, pos[2]);
    InsertNextValueIfNotNull(this->Reflectance, point.reflectance);
    InsertNextValueIfNotNull(this->Timestamp, point.timestamp * ::US_TO_MS_FACTOR);
  }
}

//-----------------------------------------------------------------------------
bool vtkTimeBasedLidarPacketInterpreter::IsLidarPacket(unsigned char const* vtkNotUsed(data),
  unsigned int dataLength)
{
  return dataLength == sizeof(PacketType);
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkTimeBasedLidarPacketInterpreter::CreateNewEmptyFrame(
  vtkIdType nbrOfPoints,
  vtkIdType prereservedNbrOfPoints)
{
  const int defaultPrereservedNbrOfPointsPerFrame = 60000;
  // prereserve for 50% points more than actually received in previous frame
  prereservedNbrOfPoints =
    std::max(static_cast<int>(prereservedNbrOfPoints * 1.5), defaultPrereservedNbrOfPointsPerFrame);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();

  // Initialize points
  vtkNew<vtkPoints> points;
  points->SetDataTypeToFloat();
  points->Allocate(prereservedNbrOfPoints);
  if (nbrOfPoints > 0)
  {
    points->SetNumberOfPoints(nbrOfPoints);
  }
  points->GetData()->SetName("Points");
  polyData->SetPoints(points.GetPointer());

  // Initialize data arrays
  this->Points = points.GetPointer();
  // clang-format off
  InitArrayForPolyData(true, this->X, "X", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(true, this->Y, "Y", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(true, this->Z, "Z", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(false, this->Reflectance, "reflectance", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(false, this->Timestamp, "timestamp", nbrOfPoints, prereservedNbrOfPoints, polyData);
  // clang-format on

  // Set the default array to display in the application
  polyData->GetPointData()->SetActiveScalars("reflectance");

  return polyData;
}

//-----------------------------------------------------------------------------
bool vtkTimeBasedLidarPacketInterpreter::PreProcessPacket(unsigned char const* data,
  unsigned int vtkNotUsed(dataLength),
  double& outLidarDataTime)
{
  const PacketType* dataPacket = reinterpret_cast<const PacketType*>(data);

  outLidarDataTime = dataPacket->data[0].timestamp * ::US_TO_MS_FACTOR;
  return this->DoSplitPacket(outLidarDataTime);
}

//-----------------------------------------------------------------------------
void vtkTimeBasedLidarPacketInterpreter::Initialize()
{
  this->LastTimestamp = std::numeric_limits<double>::max();
  Superclass::Initialize();
}

//-----------------------------------------------------------------------------
bool vtkTimeBasedLidarPacketInterpreter::DoSplitPacket(double timestamp)
{
  // In case the pcap was payed backward
  if (this->LastTimestamp > timestamp ||
    timestamp - this->LastTimestamp > this->PublishInterval * 2U)
  {
    this->LastTimestamp = timestamp;
    return false;
  }

  // Not enough time between two frames
  if (timestamp - this->LastTimestamp < this->PublishInterval)
  {
    return false;
  }
  this->LastTimestamp = timestamp;
  return true;
}