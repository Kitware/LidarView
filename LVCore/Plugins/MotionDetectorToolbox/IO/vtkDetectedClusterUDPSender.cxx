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
#include <vtkUnsignedShortArray.h>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <algorithm>
#include <limits>

vtkStandardNewMacro(vtkDetectedClusterUDPSender);

namespace
{
constexpr uint16_t PACKET_SIZE = 508;
constexpr uint8_t FOOTER_SIZE = sizeof(uint16_t) * 2;
constexpr uint8_t DATA_START[2] = { 0x28, 0x2a };
constexpr uint8_t PACKET_END[2] = { 0x2a, 0x2c };
constexpr uint16_t FIELD_DATA_BLOCK_SIZE = 30;

//-----------------------------------------------------------------------------
uint16_t CopyData(uint8_t* dataBuffer,
  uint16_t dataIndex,
  const void* fromData,
  size_t fromDataSize)
{
  std::memcpy(dataBuffer + dataIndex, fromData, fromDataSize);
  return dataIndex + fromDataSize;
}

//-----------------------------------------------------------------------------
template <typename Type, uint8_t N>
void CopyArray(vtkFieldData* fieldData, const char* name, uint8_t* dataBuffer, uint16_t& currentIdx)
{
  typename Type::ValueType data[N] = { 0 };

  auto array = Type::SafeDownCast(fieldData->GetAbstractArray(name));
  if (array && array->GetNumberOfComponents() == N && array->GetNumberOfTuples() == 1)
  {
    array->GetTypedTuple(0, data);
  }
  currentIdx = ::CopyData(dataBuffer, currentIdx, &data, sizeof(data));
}
}

//-----------------------------------------------------------------------------
class vtkDetectedClusterUDPSender::vtkInternals
{
public:
  boost::asio::io_service IOService;
  boost::asio::ip::udp::socket Socket;
  boost::asio::ip::udp::endpoint Endpoint;

  uint8_t DataBuffer[::PACKET_SIZE];

  vtkInternals()
    : Socket(IOService)
  {
  }

  //-----------------------------------------------------------------------------
  uint16_t CopyData(uint16_t dataIndex, const void* fromData, size_t fromDataSize)
  {
    return ::CopyData(this->DataBuffer, dataIndex, fromData, fromDataSize);
  }

  //-----------------------------------------------------------------------------
  uint16_t ResetPayload()
  {
    std::fill(this->DataBuffer, this->DataBuffer + ::PACKET_SIZE, 0);

    uint16_t currentIdx = 0;
    currentIdx = this->CopyData(currentIdx, &DATA_START, sizeof(::DATA_START));
    // Reserve a byte for block number
    return currentIdx + 2;
  }

  //-----------------------------------------------------------------------------
  void FillBlock(vtkFieldData* fieldData, uint16_t& currentIdx)
  {
    ::CopyArray<vtkUnsignedShortArray, 1>(fieldData, "Label", this->DataBuffer, currentIdx);
    ::CopyArray<vtkFloatArray, 3>(fieldData, "Center", this->DataBuffer, currentIdx);
    ::CopyArray<vtkFloatArray, 1>(fieldData, "Distance", this->DataBuffer, currentIdx);
    ::CopyArray<vtkFloatArray, 3>(fieldData, "Size", this->DataBuffer, currentIdx);
  }

  //-----------------------------------------------------------------------------
  void SendPacket(uint16_t nbOfBlocks, uint16_t currentIdx)
  {
    this->CopyData(2, &nbOfBlocks, sizeof(uint16_t));

    const uint16_t packetRealSize = currentIdx + ::FOOTER_SIZE;
    currentIdx = this->CopyData(currentIdx, &packetRealSize, sizeof(uint16_t));
    this->CopyData(currentIdx, &::PACKET_END, sizeof(uint16_t));
    this->Socket.send_to(boost::asio::buffer(this->DataBuffer, ::PACKET_SIZE), this->Endpoint);
  }

  //-----------------------------------------------------------------------------
  void SendData(vtkCompositeDataSet* blocks)
  {
    this->ResetPayload();
    uint16_t currentIdx = this->ResetPayload();
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
        currentIdx = this->ResetPayload();
        nbOfBlocks = 0;
      }
      this->FillBlock(fieldData, currentIdx);
      ++nbOfBlocks;
    }

    if (nbOfBlocks)
    {
      this->SendPacket(nbOfBlocks, currentIdx);
    }
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
int vtkDetectedClusterUDPSender::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Enabled)
  {
    return 1;
  }

  auto& internals = *(this->Internals);

  boost::system::error_code error;
  auto address = boost::asio::ip::make_address(this->IPAddress, error);
  if (error)
  {
    vtkErrorMacro("Could not determine a valid address from input string.");
    return 0;
  }

  internals.Endpoint = boost::asio::ip::udp::endpoint(address, this->DestinationPort);
  return 1;
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

  auto& internals = *(this->Internals);

  if (!this->Enabled)
  {
    return 1;
  }

  boost::system::error_code error;
  internals.Socket.open(internals.Endpoint.protocol(), error);

  if (error)
  {
    vtkErrorMacro("Could not open socket.");
    return 0;
  }
  internals.Socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  internals.Socket.set_option(boost::asio::ip::multicast::enable_loopback(true));

  internals.SendData(input);

  internals.Socket.close();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkDetectedClusterUDPSender::FillInputPortInformation(int vtkNotUsed(port),
  vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkDetectedClusterUDPSender::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IPAddress: " << this->IPAddress << endl;
  os << indent << "DestinationPort: " << this->DestinationPort << endl;
}
