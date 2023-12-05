/*=========================================================================

  Program:   LidarView
  Module:    vtkExamplePacketInterpreter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExamplePacketInterpreter.h"
#include "InterpreterHelper.h"

#include "ExampleFormat.h"

#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkTransform.h>

#include <filesystem>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkExamplePacketInterpreter)

//-----------------------------------------------------------------------------
vtkExamplePacketInterpreter::vtkExamplePacketInterpreter()
{
  this->ParserMetaData.SpecificInformation = std::make_shared<ExampleSpecificFrameInformation>();

  this->ResetCurrentFrame();
}

//-----------------------------------------------------------------------------
vtkExamplePacketInterpreter::~vtkExamplePacketInterpreter()
{
  // delete this->CurrentFrameState;
}

//-----------------------------------------------------------------------------
void vtkExamplePacketInterpreter::ProcessPacket(unsigned char const* data, unsigned int dataLength)
{
  if (!this->IsLidarPacket(data, dataLength))
  {
    return;
  }

  ExampleSpecificFrameInformation* frameInfo = reinterpret_cast<ExampleSpecificFrameInformation*>(
    this->ParserMetaData.SpecificInformation.get());

  const LidarPacket* dataPacket = reinterpret_cast<const LidarPacket*>(data);

  // Detection of a new frame
  // In this example ...

  if (dataPacket->header.GetFrameID() != frameInfo->frameID)
  {
    this->SplitFrame();
    this->LastTimestamp = std::numeric_limits<unsigned int>::max();
  }

  // Update the transforms here and then call internal transform
  if (this->SensorTransform)
  {
    this->SensorTransform->Update();
  }

  for (unsigned int i = 0; i < numberOfChannels; i++)
  {
    double pos[3] = { 0, 0, 0 };

    // Apply sensor transform
    if (this->SensorTransform)
    {
      this->SensorTransform->InternalTransformPoint(pos, pos);
    }

    this->Points->InsertNextPoint(pos);
    InsertNextValueIfNotNull(this->PointsX, pos[0]);
    InsertNextValueIfNotNull(this->PointsY, pos[1]);
    InsertNextValueIfNotNull(this->PointsZ, pos[2]);
    InsertNextValueIfNotNull(this->Azimuth, 0);
    InsertNextValueIfNotNull(this->Intensity, 0);
    InsertNextValueIfNotNull(this->LaserId, 0);
    InsertNextValueIfNotNull(this->Timestamp, 0);
    InsertNextValueIfNotNull(this->Distance, 0);
  }
}

//-----------------------------------------------------------------------------
bool vtkExamplePacketInterpreter::IsLidarPacket(unsigned char const* vtkNotUsed(data),
  unsigned int dataLength)
{
  return dataLength == sizeof(FiringBlock);
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkExamplePacketInterpreter::CreateNewEmptyFrame(vtkIdType nbrOfPoints,
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
  InitArrayForPolyData(true, this->PointsX, "X", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(true, this->PointsY, "Y", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(true, this->PointsZ, "Z", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(
    false, this->Azimuth, "azimuth", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(
    false, this->Intensity, "intensity", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(
    false, this->LaserId, "laser_id", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(
    false, this->Timestamp, "Timestamp", nbrOfPoints, prereservedNbrOfPoints, polyData);
  InitArrayForPolyData(
    false, this->Distance, "distance_m", nbrOfPoints, prereservedNbrOfPoints, polyData);

  // Set the default array to display in the application
  polyData->GetPointData()->SetActiveScalars("intensity");

  return polyData;
}

//-----------------------------------------------------------------------------
bool vtkExamplePacketInterpreter::PreProcessPacket(unsigned char const* data,
  unsigned int vtkNotUsed(dataLength),
  fpos_t filePosition,
  double packetNetworkTime,
  std::vector<FrameInformation>* vtkNotUsed(frameCatalog))
{
  const LidarPacket* dataPacket = reinterpret_cast<const LidarPacket*>(data);

  ExampleSpecificFrameInformation* frameInfo = reinterpret_cast<ExampleSpecificFrameInformation*>(
    this->ParserMetaData.SpecificInformation.get());

  // Detection of a new frame
  // In this example a frame is detected thanks to a fiel "frame id" in the header of the packet.
  // This means that in this example there is no new frame detected inside a unique packet.
  bool isNewFrame = (dataPacket->header.GetFrameID() != frameInfo->frameID);
  if (isNewFrame)
  {
    // A new frame is detected, we flush the previous one
    this->SplitFrame();

    // We also update the information for the new frame
    // You need to set the file position to determine the beginning of a frame
    this->ParserMetaData.FilePosition = filePosition;
    this->ParserMetaData.FirstPacketDataTime = this->ParserMetaData.FirstPacketNetworkTime =
      packetNetworkTime;
  }

  return isNewFrame;
}

//-----------------------------------------------------------------------------
std::string vtkExamplePacketInterpreter::GetSensorInformation(bool vtkNotUsed(shortVersion))
{
  return std::string("Example Sensor");
}

//-----------------------------------------------------------------------------
std::string vtkExamplePacketInterpreter::GetSensorName()
{
  return "ExampleSensor" + std::to_string(this->GetNumberOfChannels());
}

void vtkExamplePacketInterpreter::LoadCalibration(const std::string& filename)
{
  const auto path = std::filesystem::path(filename);
  if (filename.empty() || path.extension() != ".csv")
  {
    this->IsCalibrated = false;
    return;
  }
}