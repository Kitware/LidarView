/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPacketInterpreter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarPacketInterpreter.h"

#include <vtkDoubleArray.h>
#include <vtkFieldData.h>

#include <ctime>
#include <sstream>

namespace
{
//-----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> NewVertexCells(vtkIdType numberOfVerts)
{
  vtkNew<vtkIdTypeArray> cells;
  cells->SetNumberOfValues(numberOfVerts * 2);
  vtkIdType* ids = cells->GetPointer(0);
  for (vtkIdType i = 0; i < numberOfVerts; ++i)
  {
    ids[i * 2] = 1;
    ids[i * 2 + 1] = i;
  }

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->SetCells(numberOfVerts, cells.GetPointer());
  return cellArray;
}
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::SplitFrame(bool force,
  FramingMethod_t framingMethodAskingForSplitFrame)
{
  if ((force || this->FramingMethod == framingMethodAskingForSplitFrame) && this->CurrentFrame)
  {
    const vtkIdType nPtsOfCurrentDataset = this->CurrentFrame->GetNumberOfPoints();
    if (this->IgnoreEmptyFrames && (nPtsOfCurrentDataset == 0) && !force)
    {
      return false;
    }

    // Create a field data array with RPM info
    const std::string fsNameArray[2] = { "RPM", "FPS" };
    double fsValues[2];
    fsValues[0] = this->GetRpm();
    fsValues[1] = this->GetFrequency();
    for (unsigned int i = 0; i < 2; i++)
    {
      if (fsValues[i] != 0)
      {
        vtkDoubleArray* fsArray = vtkDoubleArray::New();
        fsArray->SetName(fsNameArray[i].c_str());
        fsArray->SetNumberOfComponents(1);
        fsArray->SetNumberOfTuples(1);
        fsArray->InsertComponent(0, 0, fsValues[i]);
        this->CurrentFrame->GetFieldData()->AddArray(fsArray);
        fsArray->Delete();
      }
    }

    // add vertex to the polydata
    this->CurrentFrame->SetVerts(NewVertexCells(this->CurrentFrame->GetNumberOfPoints()));
    // split the frame
    this->Frames.push_back(this->CurrentFrame);
    // create a new frame
    this->CurrentFrame = this->CreateNewEmptyFrame(0, nPtsOfCurrentDataset);

    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::SetLaserSelection(int index, int value)
{
  if ((index < 0) || (index >= this->LaserSelection->GetNumberOfTuples()))
  {
    vtkErrorMacro(<< "Bad mode index: " << index);
  }

  this->LaserSelection->SetTuple1(index, value);
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkIntArray* vtkLidarPacketInterpreter::GetLaserSelection()
{
  return this->LaserSelection.GetPointer();
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::IsNewData()
{
  return this->IsNewFrameReady();
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::IsValidPacket(unsigned char const* data, unsigned int dataLength)
{
  return this->IsLidarPacket(data, dataLength);
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::ResetCurrentData()
{
  this->ResetCurrentFrame();
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::ProcessPacketWrapped(unsigned char const* data,
  unsigned int dataLength,
  double PacketNetworkTime_s)
{
  // if the framing Method is the NetworkPacketTime one
  // We check if the frame has te be split.
  if (this->IsLidarPacket(data, dataLength) &&
    this->FramingMethod == FramingMethod_t::NETWORK_PACKET_TIME_FRAMING)
  {
    auto currentFrameNumber =
      static_cast<unsigned long long>(PacketNetworkTime_s / this->FrameDuration_s);
    if (this->LastNetworkTimeFrameNumber != 0 // do not frame on first call of this function
      && currentFrameNumber != this->LastNetworkTimeFrameNumber) // new frame found
    {
      this->SplitFrame(false, FramingMethod_t::NETWORK_PACKET_TIME_FRAMING);
    }
    this->LastNetworkTimeFrameNumber = currentFrameNumber;
  }

  // Interpreter the packet
  this->ProcessPacket(data, dataLength);
}

//-----------------------------------------------------------------------------
bool vtkLidarPacketInterpreter::PreProcessPacketWrapped(unsigned char const* data,
  unsigned int dataLength,
  fpos_t filePosition,
  double packetNetworkTime_s,
  std::vector<FrameInformation>* frameCatalog)
{
  // If the framing method is the interpreter one
  // frameCatalog is filled with the PreProcessPacket function
  if (this->FramingMethod == FramingMethod_t::INTERPRETER_FRAMING)
  {
    return this->PreProcessPacket(
      data, dataLength, filePosition, packetNetworkTime_s, frameCatalog);
  }

  // If the framing method is the network time packet one,
  // frameCatalog is filled every frameDuration time.
  if (this->FramingMethod == FramingMethod_t::NETWORK_PACKET_TIME_FRAMING)
  {
    unsigned long long currentFrameNumber =
      static_cast<unsigned long long>(packetNetworkTime_s / this->FrameDuration_s);
    if (currentFrameNumber != this->previousFrameNumber)
    {
      FrameInformation information;
      information.FilePosition = filePosition;
      information.FirstPacketNetworkTime = packetNetworkTime_s;
      // FirstPacketDataTime is not well-defined, because we do not parse
      // the data and thus cannot get data time, however providing a
      // plausible value can help prevent breaking some algorithms.
      // Possible improvement: first pass using INTERPRETER_FRAMING to
      // compute the timeshift, then apply it to FirstPacketNetworkTime
      information.FirstPacketDataTime = packetNetworkTime_s;
      frameCatalog->push_back(information);
      this->previousFrameNumber = currentFrameNumber;
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
std::string vtkLidarPacketInterpreter::GetDefaultRecordFileName()
{
  std::stringstream defaultFileName;

  // Add time string YYYY-mm-dd-HH-MM-SS
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  defaultFileName << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");

  return defaultFileName.str();
}
