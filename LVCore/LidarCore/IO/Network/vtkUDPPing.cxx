/*=========================================================================

  Program: LidarView
  Module:  vtkUDPPing.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUDPPing.h"

#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <algorithm>
#include <chrono>
#include <limits>

vtkStandardNewMacro(vtkUDPPing);

namespace
{
constexpr uint16_t PACKET_SIZE = 12;
constexpr uint8_t PACKET_START[2] = { 0x28, 0x2a };
constexpr uint8_t PACKET_END[2] = { 0x2a, 0x2c };
}

//-----------------------------------------------------------------------------
class vtkUDPPing::vtkInternals
{
public:
  uint8_t DataBuffer[::PACKET_SIZE];

  std::chrono::system_clock::time_point LastSent;

  vtkInternals()
    : LastSent(std::chrono::system_clock::now())
  {
  }

  //-----------------------------------------------------------------------------
  uint16_t CopyData(uint16_t index, const void* data, size_t dataSize)
  {
    return Superclass::CopyData(this->DataBuffer, index, data, dataSize);
  }
};

//-----------------------------------------------------------------------------
vtkUDPPing::vtkUDPPing()
  : Internals(new vtkUDPPing::vtkInternals())
{
}

//-----------------------------------------------------------------------------
vtkUDPPing::~vtkUDPPing() = default;

//-----------------------------------------------------------------------------
int vtkUDPPing::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Pass input to output without any changes
  vtkDataSet* output = vtkDataSet::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(input);

  auto& internals = *(this->Internals);

  auto now = std::chrono::system_clock::now();

  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - internals.LastSent);
  if (!Superclass::GetEnabled() || elapsed.count() < this->MinimalDelay)
  {
    return 1;
  }
  internals.LastSent = now;

  if (!Superclass::OpenSocket())
  {
    return 0;
  }

  // Reset all bytes in buffer
  std::fill(internals.DataBuffer, internals.DataBuffer + ::PACKET_SIZE, 0);

  internals.CopyData(0, &::PACKET_START, sizeof(uint16_t));
  uint64_t nbOfPoints = static_cast<uint64_t>(input->GetNumberOfPoints());
  internals.CopyData(2, &nbOfPoints, sizeof(uint64_t));
  internals.CopyData(10, &::PACKET_END, sizeof(uint16_t));

  Superclass::SendData(internals.DataBuffer, ::PACKET_SIZE);

  Superclass::CloseSocket();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkUDPPing::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkUDPPing::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MinimalDelay: " << this->MinimalDelay << endl;
}
