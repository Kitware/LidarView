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
#include <vtkStringArray.h>
#include <vtkTransform.h>

#include <ctime>
#include <sstream>

namespace
{
constexpr const char* SPEED_FIELD_DATA_NAME[2] = { "RPM", "FPS" };
constexpr const char* INFO_FIELD_DATA_NAME[2] = { "Vendor", "Model" };

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
void vtkLidarPacketInterpreter::Initialize()
{
  this->IsInitialized = true;
  // AdvancedArrays could have been changed so we need to reset current frame.
  this->ResetCurrentFrame();
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::ResetInitializedState()
{
  this->IsInitialized = false;
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
    double fsValues[2];
    fsValues[0] = this->GetRpm();
    fsValues[1] = this->GetFrequency();
    for (unsigned int i = 0; i < 2; i++)
    {
      if (fsValues[i] != 0)
      {
        vtkSmartPointer<vtkDoubleArray> fsArray = vtkSmartPointer<vtkDoubleArray>::New();
        fsArray->SetName(::SPEED_FIELD_DATA_NAME[i]);
        fsArray->SetNumberOfComponents(1);
        fsArray->SetNumberOfTuples(1);
        fsArray->InsertComponent(0, 0, fsValues[i]);
        this->CurrentFrame->GetFieldData()->AddArray(fsArray);
      }
    }

    // Add sensor name and model if implemented in subclasses
    std::string infoValues[2];
    infoValues[0] = this->GetSensorVendor();
    infoValues[1] = this->GetSensorModelName();
    for (unsigned int idx = 0; idx < 2; idx++)
    {
      if (!infoValues[idx].empty())
      {
        vtkSmartPointer<vtkStringArray> strArray = vtkSmartPointer<vtkStringArray>::New();
        strArray->SetName(::INFO_FIELD_DATA_NAME[idx]);
        strArray->SetNumberOfComponents(1);
        strArray->SetNumberOfTuples(1);
        strArray->InsertValue(0, infoValues[idx].c_str());
        this->CurrentFrame->GetFieldData()->AddArray(strArray);
      }
    }

    // Apply transform on all points
    if (this->GetSensorTransform())
    {
      vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
      newPts->Allocate(this->CurrentFrame->GetNumberOfPoints());
      newPts->GetData()->SetName(this->CurrentFrame->GetPoints()->GetData()->GetName());
      this->GetSensorTransform()->TransformPoints(this->CurrentFrame->GetPoints(), newPts);
      this->CurrentFrame->SetPoints(newPts);
    }

    // add vertex to the polydata
    this->CurrentFrame->SetVerts(::NewVertexCells(this->CurrentFrame->GetNumberOfPoints()));
    // free extra memory allocated
    this->CurrentFrame->Squeeze();

    // split the frame
    this->Frames.push_back(this->CurrentFrame);
    // create a new frame
    this->CurrentFrame = this->CreateNewEmptyFrame(0, nPtsOfCurrentDataset);

    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkLidarPacketInterpreter::SetCalibrationFileName(const char* filename)
{
  if (this->CalibrationFileName && filename && strcmp(this->CalibrationFileName, filename) != 0)
  {
    this->ResetInitializedState();
  }
  vtkSetStringBodyMacro(CalibrationFileName, filename);
};

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
  double packetNetworkTime,
  double& outLidarDataTime)
{
  switch (this->FramingMethod)
  {
    case FramingMethod_t::NETWORK_PACKET_TIME_FRAMING:
    {
      unsigned long long currentFrameNumber =
        static_cast<unsigned long long>(packetNetworkTime / this->FrameDuration_s);
      if (currentFrameNumber != this->previousFrameNumber)
      {
        // FirstPacketDataTime is not well-defined, because we do not parse
        // the data and thus cannot get data time, however providing a
        // plausible value can help prevent breaking some algorithms.
        // Possible improvement: first pass using INTERPRETER_FRAMING to
        // compute the time shift, then apply it to packetNetworkTime
        outLidarDataTime = packetNetworkTime;
        this->previousFrameNumber = currentFrameNumber;
        return true;
      }
      break;
    }

    default:
    case FramingMethod_t::INTERPRETER_FRAMING:
      return this->PreProcessPacket(data, dataLength, outLidarDataTime);
  }
  return false;
}

//-----------------------------------------------------------------------------
std::string vtkLidarPacketInterpreter::GetSensorInformation(bool vtkNotUsed(shortVersion))
{
  return this->GetSensorVendor() + " - " + this->GetSensorModelName();
}
