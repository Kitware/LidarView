/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPoseReader.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarPoseReader.h"
#include "vtkHelper.h"

#include <vtkCellArray.h>
#include <vtkInformation.h>
#include <vtkLine.h>
#include <vtkNew.h>
#include <vtkPolyLine.h>

namespace
{
constexpr unsigned int POSITION_ORIENTATION_PORT = 2;
constexpr unsigned int POSE_RAW_INFORMATION_PORT = 3;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarPoseReader)

//-----------------------------------------------------------------------------
vtkLidarPoseReader::vtkLidarPoseReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(4);
}

//-----------------------------------------------------------------------------
int vtkLidarPoseReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == ::POSITION_ORIENTATION_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == ::POSE_RAW_INFORMATION_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  return vtkLidarReader::FillOutputPortInformation(port, info);
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkLidarPoseReader::GetMTime()
{
  if (this->PoseInterpreter)
  {
    return std::max(this->Superclass::GetMTime(), this->PoseInterpreter->GetMTime());
  }
  return this->Superclass::GetMTime();
}

//-----------------------------------------------------------------------------
bool vtkLidarPoseReader::Open()
{
  std::vector<int> ports;
  if (this->GetLidarPort() != -1)
  {
    ports.emplace_back(this->GetLidarPort());
    ports.emplace_back(this->LidarPosePort);
  }
  return Superclass::Open(ports);
}

//-----------------------------------------------------------------------------
bool vtkLidarPoseReader::Open(bool vtkNotUsed(reassemble))
{
  return this->Open();
}

//-----------------------------------------------------------------------------
void vtkLidarPoseReader::ReadPoses()
{
  if (!this->Open())
  {
    vtkErrorMacro("ReadPoses() called but packet file reader is not open.");
    return;
  }

  double timeSinceStart;

  while (this->ReadNextPacket(timeSinceStart))
  {
    const std::vector<uint8_t>& payload = this->GetPayload();
    // If the current packet is not valid,
    // skip it and update the file position
    if (!this->PoseInterpreter->IsValidPacket(payload.data(), payload.size()))
    {
      continue;
    }

    // Process the packet
    this->PoseInterpreter->ProcessPacket(payload.data(), payload.size());
  }
  this->Close();

  // Save positions and orientation information if packets contain them
  if (this->PoseInterpreter->HasPoseInformation())
  {
    this->PoseInfo = this->PoseInterpreter->GetPose();

    // Set the polyline to the poly data to see the pose information
    vtkSmartPointer<vtkPolyLine> polyLine = CreatePolyLineFromPoints(this->PoseInfo->GetPoints());
    vtkNew<vtkCellArray> cellArray;
    cellArray->InsertNextCell(polyLine);
    this->PoseInfo->SetLines(cellArray);

    this->PoseInterpreter->FillInterpolatorFromPose();
  }

  // Save raw information if packets contain them
  if (this->PoseInterpreter->HasRawInformation())
  {
    this->RawInfos = this->PoseInterpreter->GetRawInformation();
  }
}

//-----------------------------------------------------------------------------
int vtkLidarPoseReader::RequestInformation(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->PoseInterpreter)
  {
    vtkErrorMacro("No pose interpreter selected.");
    return 0;
  }

  if (this->GetFileName().empty())
  {
    vtkErrorMacro("FileName has not been set.");
    return 0;
  }

  this->ReadLidarPoseData = true;
  return vtkLidarReader::RequestInformation(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkLidarPoseReader::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* outputPositionOrientation =
    vtkPolyData::GetData(outputVector, ::POSITION_ORIENTATION_PORT);
  vtkTable* outputRawInformation = vtkTable::GetData(outputVector, ::POSE_RAW_INFORMATION_PORT);

  if (this->ReadLidarPoseData)
  {
    this->PoseInterpreter->ResetCurrentData();
    this->ReadPoses();
    this->ReadLidarPoseData = false;
  }

  if (this->PoseInfo)
  {
    outputPositionOrientation->ShallowCopy(this->PoseInfo);
  }
  if (this->RawInfos)
  {
    outputRawInformation->ShallowCopy(this->RawInfos);
  }

  return vtkLidarReader::RequestData(request, inputVector, outputVector);
}
