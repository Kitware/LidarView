/*=========================================================================

  Program:   LidarView
  Module:    vtkRotativeLidarPacketInterpreter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRotativeLidarPacketInterpreter.h"
#include "InterpreterHelper.h"

#include "RotativeLidarFormat.h"

#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>

#include <filesystem>

namespace
{
constexpr const char* MODEL_NAME[2] = { "ExampleModel16", "ExampleModel32" };
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkRotativeLidarPacketInterpreter)

//-----------------------------------------------------------------------------
vtkRotativeLidarPacketInterpreter::vtkRotativeLidarPacketInterpreter()
{
  // Theses information will be stored in point cloud field data
  this->SetSensorVendor("Kitware");
  this->SetSensorModelName("Example");

  this->ResetCurrentFrame();
}

//-----------------------------------------------------------------------------
vtkRotativeLidarPacketInterpreter::~vtkRotativeLidarPacketInterpreter() = default;

//-----------------------------------------------------------------------------
void vtkRotativeLidarPacketInterpreter::ProcessPacket(unsigned char const* data,
  unsigned int dataLength)
{
  if (!this->IsLidarPacket(data, dataLength))
  {
    return;
  }

  const LidarPacket* dataPacket = reinterpret_cast<const LidarPacket*>(data);

  // Detection of a new frame
  bool isNewFrame = (dataPacket->header.GetFrameID() != this->LastFrameID);
  this->LastFrameID = dataPacket->header.GetFrameID();
  if (isNewFrame)
  {
    this->SplitFrame();
    this->LastTimestamp = std::numeric_limits<unsigned int>::max();
  }

  for (unsigned int i = 0; i < numberOfChannels; i++)
  {
    double pos[3] = { 0, 0, 0 };

    this->Points->InsertNextPoint(pos);
    InsertNextValueIfNotNull(this->PointsX, pos[0]);
    InsertNextValueIfNotNull(this->PointsY, pos[1]);
    InsertNextValueIfNotNull(this->PointsZ, pos[2]);
    InsertNextValueIfNotNull(this->Intensity, 0);
    InsertNextValueIfNotNull(this->LaserId, 0);
    InsertNextValueIfNotNull(this->Timestamp, 0);
    InsertNextValueIfNotNull(this->Distance, 0);
  }
}

//-----------------------------------------------------------------------------
bool vtkRotativeLidarPacketInterpreter::IsLidarPacket(unsigned char const* vtkNotUsed(data),
  unsigned int dataLength)
{
  return dataLength == sizeof(FiringBlock);
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkRotativeLidarPacketInterpreter::CreateNewEmptyFrame(
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
  InitArrayForPolyData(true, this->PointsX, "X", nbrOfPoints, prereservedNbrOfPoints, polyData, this->EnableAdvancedArrays);
  InitArrayForPolyData(true, this->PointsY, "Y", nbrOfPoints, prereservedNbrOfPoints, polyData, this->EnableAdvancedArrays);
  InitArrayForPolyData(true, this->PointsZ, "Z", nbrOfPoints, prereservedNbrOfPoints, polyData, this->EnableAdvancedArrays);
  InitArrayForPolyData(false, this->Intensity, "intensity", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(false, this->LaserId, "laser_id", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(false, this->Timestamp, "timestamp", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(false, this->Distance, "distance_m", nbrOfPoints, prereservedNbrOfPoints, polyData);
  // clang-format on

  // Set the default array to display in the application
  polyData->GetPointData()->SetActiveScalars("intensity");

  return polyData;
}

//-----------------------------------------------------------------------------
bool vtkRotativeLidarPacketInterpreter::PreProcessPacket(unsigned char const* data,
  unsigned int vtkNotUsed(dataLength),
  double& outLidarDataTime)
{
  const LidarPacket* dataPacket = reinterpret_cast<const LidarPacket*>(data);

  outLidarDataTime = dataPacket->header.GetTimestamp();
  // Detection of a new frame
  // In this example a frame is detected thanks to a fiel "frame id" in the header of the packet.
  // This means that in this example there is no new frame detected inside a unique packet.
  bool isNewFrame = (dataPacket->header.GetFrameID() != this->LastFrameID);
  this->LastFrameID = dataPacket->header.GetFrameID();
  return isNewFrame;
}

//-----------------------------------------------------------------------------
void vtkRotativeLidarPacketInterpreter::Initialize()
{
  std::string filename;
  if (this->CalibrationFileName != nullptr)
  {
    filename = this->CalibrationFileName;
  }

  const auto path = std::filesystem::path(filename);
  if (filename.empty() || path.extension() != ".csv")
  {
    return;
  }
  Superclass::Initialize();
}

//-----------------------------------------------------------------------------
void vtkRotativeLidarPacketInterpreter::SetLidarModel(int model)
{
  if (this->LidarModel == model)
  {
    return;
  }

  this->LidarModel = model;
  this->SetSensorModelName(::MODEL_NAME[model]);
  this->Modified();
  this->ResetInitializedState();
}
