/*=========================================================================

  Program: LidarView
  Module:  vtkLidarStream.cxx

  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This needs to be included first because vtkSystemIncludes.h is included
// instead of stdio.h so the CMake variable __USE_LARGEFILE64 must be consistent.
// Otherwise fpos_t has 2 differents definition _G_fpos_t and _G_fpos64_t
#include "vtkLidarPacketInterpreter.h"

#include "vtkLidarStream.h"

#include <sstream>

#include <vtkCommand.h>
#include <vtkLogger.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarStream)

//-----------------------------------------------------------------------------
int vtkLidarStream::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
std::string vtkLidarStream::GetSensorInformation(bool shortVersion)
{
  return this->GetLidarInterpreter()->GetSensorInformation(shortVersion);
}

//-----------------------------------------------------------------------------
vtkLidarStream::vtkLidarStream()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
vtkLidarStream::~vtkLidarStream()
{
  // see the explanation about why this is needed in vtkStream::~vtkStream
  this->Stop();
  this->SetLidarInterpreter(nullptr);
}

//----------------------------------------------------------------------------
int vtkLidarStream::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  int numberOfFrameAvailable = 0;
  {
    std::lock_guard<std::mutex> lock(this->DataMutex);
    numberOfFrameAvailable = this->CheckForNewData();
    if (numberOfFrameAvailable != 0)
    {
      vtkSmartPointer<vtkPolyData> polyData = this->Frames.back();
      output->ShallowCopy(polyData);
      this->Frames.clear();
      this->LastFrameProcessed += numberOfFrameAvailable;
    }
  }

  if (this->DetectFrameDropping)
  {
    if (numberOfFrameAvailable > 1)
    {
      std::stringstream text;
      text << "WARNING : At frame " << std::right << std::setw(6) << this->LastFrameProcessed
           << " Drop " << std::right << std::setw(2) << numberOfFrameAvailable - 1 << " frame(s)\n";
      vtkWarningMacro(<< text.str());
    }
  }

  vtkTable* calibration = vtkTable::GetData(outputVector, 1);
  calibration->ShallowCopy(this->GetLidarInterpreter()->GetCalibrationTable());

  return 1;
}

//----------------------------------------------------------------------------
void vtkLidarStream::Start()
{
  if (!this->GetLidarInterpreter())
  {
    vtkErrorMacro("No packet interpreter selected.");
    return;
  }
  if (!this->GetLidarInterpreter()->GetIsInitialized())
  {
    this->GetLidarInterpreter()->Initialize();
  }

  vtkStream::Start();
}

//----------------------------------------------------------------------------
void vtkLidarStream::ConsumePacket(const std::vector<uint8_t>& pkt, double timestamp)
{
  if (!this->GetLidarInterpreter())
  {
    vtkErrorMacro("No packet interpreter selected.");
    return;
  }

  if (this->PacketReceivedMap.find(pkt.size()) != this->PacketReceivedMap.cend())
  {
    this->PacketReceivedMap[pkt.size()]++;
  }
  else
  {
    this->PacketReceivedMap[pkt.size()] = 1;
  }
  if (this->NbOfPacketReceived % 10000 == 0)
  {
    std::stringstream ss;
    ss << "Received packets";
    for (auto& [size, number] : this->PacketReceivedMap)
    {
      ss << ", " << number << " packets with a size of " << size;
    }
    vtkLog(INFO, << ss.str());
    this->PacketReceivedMap.clear();
  }
  this->NbOfPacketReceived++;

  auto interp = this->GetLidarInterpreter();
  if (!interp->IsValidPacket(pkt.data(), pkt.size()))
  {
    return;
  }

  interp->ProcessPacketWrapped(pkt.data(), pkt.size(), timestamp);
  if (interp->IsNewData())
  {
    {
      std::lock_guard<std::mutex> lock(this->DataMutex);
      this->AddNewData();
      interp->ClearAllFramesAvailable();
    }
  }
}

//----------------------------------------------------------------------------
void vtkLidarStream::AddNewData()
{
  vtkSmartPointer<vtkPolyData> LastFrame = this->GetLidarInterpreter()->GetLastFrameAvailable();
  this->Frames.push_back(LastFrame);

  // This prevents accumulating frames forever when "Pause" is toggled
  // There is little reason to use a std::deque to cache the frames, so
  // while waiting for a better fix, lets set a maximum size to the queue.
  // If this maximum size is too big (>= 100) this seems to cause a
  // memory leak.
  // TODO: investigate (not needed if a refactor removes the queue)
  // TODO: check this->Frames.size() before the push_back of the new frame
  //       to avoid adding then removing the same element
  if (this->Frames.size() > 2)
  {
    this->Frames.pop_back();
  }
}

//----------------------------------------------------------------------------
int vtkLidarStream::CheckForNewData()
{
  return this->Frames.size();
}

//----------------------------------------------------------------------------
void vtkLidarStream::OnInterpreterModifiedEvent()
{
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkLidarStream::SetLidarInterpreter(vtkLidarPacketInterpreter* interpreter)
{
  if (this->LidarInterpreter == interpreter)
  {
    return;
  }

  if (this->LidarInterpreter)
  {
    this->LidarInterpreter->RemoveObserver(this->ReaderObserverId);
  }
  vtkSetObjectBodyMacro(LidarInterpreter, vtkLidarPacketInterpreter, interpreter);
  if (this->LidarInterpreter)
  {
    this->ReaderObserverId = this->LidarInterpreter->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkLidarStream::OnInterpreterModifiedEvent);
  }
}
