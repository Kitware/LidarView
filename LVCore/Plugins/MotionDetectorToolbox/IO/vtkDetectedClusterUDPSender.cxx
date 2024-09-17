/*=========================================================================

  Program: LidarView
  Module:  vtkDetectedClusterUDPSender.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDetectedClusterUDPSender.h"

#include <vtkCompositeDataIterator.h>
#include <vtkCompositeDataSet.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnsignedShortArray.h>

#include <algorithm>
#include <limits>

vtkStandardNewMacro(vtkDetectedClusterUDPSender);

namespace
{
constexpr uint16_t PACKET_SIZE = 508;
constexpr uint8_t FOOTER_SIZE = sizeof(uint16_t) * 2;
constexpr uint8_t DATA_START[2] = { 0x28, 0x2a };
constexpr uint8_t PACKET_END[2] = { 0x2a, 0x2c };
constexpr uint16_t FIELD_DATA_BLOCK_SIZE = 50;

//-----------------------------------------------------------------------------
template <typename Type, uint8_t N>
void CopyArray(vtkFieldData* fieldData, const char* name, uint8_t* dataBuffer, uint16_t& currentIdx)
{
  typename Type::ValueType data[N] = { 0 };

  auto array = Type::SafeDownCast(fieldData->GetAbstractArray(name));
  if (!array)
  {
    vtkWarningWithObjectMacro(nullptr, "Could not find array: " << name << " in data.");
    return;
  }
  if (array->GetNumberOfComponents() == N && array->GetNumberOfTuples() == 1)
  {
    array->GetTypedTuple(0, data);
  }
  currentIdx = vtkUDPSenderAlgorithm::CopyData(dataBuffer, currentIdx, &data, sizeof(data));
}
}

//-----------------------------------------------------------------------------
class vtkDetectedClusterUDPSender::vtkInternals
{
public:
  uint8_t DataBuffer[::PACKET_SIZE];

  //-----------------------------------------------------------------------------
  uint16_t CopyData(uint16_t dataIndex, const void* fromData, size_t fromDataSize)
  {
    return vtkUDPSenderAlgorithm::CopyData(this->DataBuffer, dataIndex, fromData, fromDataSize);
  }

  //-----------------------------------------------------------------------------
  uint16_t ResetPayload(double timestamp)
  {
    std::fill(this->DataBuffer, this->DataBuffer + ::PACKET_SIZE, 0);

    uint16_t currentIdx = 0;
    currentIdx = this->CopyData(currentIdx, &DATA_START, sizeof(::DATA_START));
    currentIdx = this->CopyData(currentIdx, &timestamp, sizeof(timestamp));
    // Reserve a byte for block number
    return currentIdx + 2;
  }

  //-----------------------------------------------------------------------------
  void FillBlock(vtkFieldData* fieldData, uint16_t& currentIdx)
  {
    ::CopyArray<vtkIntArray, 1>(fieldData, "ClusterId", this->DataBuffer, currentIdx);
    ::CopyArray<vtkUnsignedShortArray, 1>(fieldData, "Label", this->DataBuffer, currentIdx);
    ::CopyArray<vtkFloatArray, 3>(fieldData, "Center", this->DataBuffer, currentIdx);
    ::CopyArray<vtkFloatArray, 1>(fieldData, "Distance", this->DataBuffer, currentIdx);
    ::CopyArray<vtkFloatArray, 3>(fieldData, "Size", this->DataBuffer, currentIdx);
    ::CopyArray<vtkFloatArray, 4>(fieldData, "Orientation", this->DataBuffer, currentIdx);
  }
};

//-----------------------------------------------------------------------------
vtkDetectedClusterUDPSender::vtkDetectedClusterUDPSender()
  : Internals(new vtkDetectedClusterUDPSender::vtkInternals())
{
}

//-----------------------------------------------------------------------------
vtkDetectedClusterUDPSender::~vtkDetectedClusterUDPSender() = default;

//-----------------------------------------------------------------------------
void vtkDetectedClusterUDPSender::SendPacket(uint16_t nbOfBlocks, uint16_t currentIdx)
{
  auto& internals = *(this->Internals);

  uint16_t nbOfBlocksIdx = sizeof(::DATA_START) + sizeof(double);
  internals.CopyData(nbOfBlocksIdx, &nbOfBlocks, sizeof(uint16_t));

  const uint16_t packetRealSize = currentIdx + ::FOOTER_SIZE;
  currentIdx = internals.CopyData(currentIdx, &packetRealSize, sizeof(uint16_t));
  internals.CopyData(currentIdx, &::PACKET_END, sizeof(uint16_t));

  Superclass::SendData(internals.DataBuffer, ::PACKET_SIZE);
}

//-----------------------------------------------------------------------------
void vtkDetectedClusterUDPSender::SendData(vtkCompositeDataSet* blocks, double timestamp)
{
  auto& internals = *(this->Internals);

  uint16_t currentIdx = internals.ResetPayload(timestamp);
  uint16_t nbOfBlocks = 0;

  vtkCompositeDataIterator* iter = blocks->NewIterator();
  iter->SkipEmptyNodesOn();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!dataset)
    {
      continue;
    }
    vtkFieldData* fieldData = dataset->GetFieldData();
    if (!fieldData)
    {
      return;
    }

    if (currentIdx + ::FOOTER_SIZE + ::FIELD_DATA_BLOCK_SIZE > ::PACKET_SIZE)
    {
      this->SendPacket(nbOfBlocks, currentIdx);
      currentIdx = internals.ResetPayload(timestamp);
      nbOfBlocks = 0;
    }
    internals.FillBlock(fieldData, currentIdx);
    ++nbOfBlocks;
  }

  if (nbOfBlocks)
  {
    this->SendPacket(nbOfBlocks, currentIdx);
  }
}

//-----------------------------------------------------------------------------
int vtkDetectedClusterUDPSender::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkCompositeDataSet* input =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Pass input to output without any changes
  vtkCompositeDataSet* output = vtkCompositeDataSet::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(input);

  if (!this->GetEnabled())
  {
    return 1;
  }

  if (!Superclass::OpenSocket())
  {
    return 0;
  }

  double requestedTime = 0.0;
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    requestedTime = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  this->SendData(input, requestedTime);

  Superclass::CloseSocket();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkDetectedClusterUDPSender::FillInputPortInformation(int vtkNotUsed(port),
  vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}
