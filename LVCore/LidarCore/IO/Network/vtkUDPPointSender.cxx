/*=========================================================================

  Program: LidarView
  Module:  vtkUDPPointSender.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUDPPointSender.h"

#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataArraySelection.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <algorithm>
#include <limits>

vtkStandardNewMacro(vtkUDPPointSender);

namespace
{
constexpr uint16_t PACKET_SIZE = 508;
constexpr uint8_t HEADER_END[2] = { 0x28, 0x2a };
constexpr uint8_t PACKET_END[2] = { 0x2a, 0x2c };
constexpr uint8_t FOOTER_SIZE = 4U;
constexpr uint8_t SCALAR_NAME_MAX_SIZE = 24U;
}

//-----------------------------------------------------------------------------
class vtkUDPPointSender::vtkInternals
{
public:
  uint8_t DataBuffer[::PACKET_SIZE];
  uint16_t MaxPointNb = 0;
  double LastValue;

  uint16_t HeaderSize;

  //-----------------------------------------------------------------------------
  uint16_t CopyData(uint16_t index, const void* data, size_t dataSize)
  {
    return vtkUDPSenderAlgorithm::CopyData(this->DataBuffer, index, data, dataSize);
  }

  //-----------------------------------------------------------------------------
  bool InitPayloadHeader(vtkFieldData* fieldData, vtkDataArraySelection* selection)
  {
    uint16_t currentIdx = 2; // Keep bytes for header size
    const bool isBigEndian = false;
    currentIdx = this->CopyData(currentIdx, &isBigEndian, sizeof(uint8_t));

    const uint8_t dataType = sizeof(float);
    currentIdx = this->CopyData(currentIdx, &dataType, sizeof(uint8_t));

    // Skip - placeholder for number of associated array
    const uint16_t numberOfArraysIndex = currentIdx;
    currentIdx += sizeof(uint8_t);

    // Init point coordinates size
    int numberOfArrays = 0;
    uint16_t pointDataSize = 3 * dataType;
    for (int idx = 0; idx < fieldData->GetNumberOfArrays(); ++idx)
    {
      auto array = vtkDataArray::SafeDownCast(fieldData->GetAbstractArray(idx));
      if (array && array->GetName())
      {
        std::string name = array->GetName();
        if (selection->ArrayIsEnabled(name.c_str()))
        {
          std::size_t sizeName = name.size();
          if (sizeName > ::SCALAR_NAME_MAX_SIZE)
          {
            vtkGenericWarningMacro("Scalar name: " << name.c_str() << " is too big (> "
                                                   << ::SCALAR_NAME_MAX_SIZE
                                                   << " char), it will be truncated.");
            sizeName = ::SCALAR_NAME_MAX_SIZE;
          }
          currentIdx = this->CopyData(currentIdx, &sizeName, sizeof(uint8_t));
          currentIdx = this->CopyData(currentIdx, name.c_str(), sizeName);

          const int nbComponents = array->GetNumberOfComponents();
          if (nbComponents > std::numeric_limits<uint8_t>::max())
          {
            vtkGenericWarningMacro("Too many components in " << name.c_str() << " array.");
            return false;
          }
          currentIdx = this->CopyData(currentIdx, &nbComponents, sizeof(uint8_t));
          pointDataSize += nbComponents * dataType;
          numberOfArrays++;
        }
      }
    }

    if (numberOfArrays > std::numeric_limits<uint8_t>::max())
    {
      vtkGenericWarningMacro("Too many arrays in input dataset.");
      return false;
    }
    this->CopyData(numberOfArraysIndex, &numberOfArrays, sizeof(uint8_t));

    // Skip - placeholder for number of point
    currentIdx += sizeof(uint16_t);

    currentIdx = this->CopyData(currentIdx, &::HEADER_END, sizeof(uint8_t) * 2);
    this->HeaderSize = currentIdx;

    this->MaxPointNb = (::PACKET_SIZE - ::FOOTER_SIZE - this->HeaderSize) / pointDataSize;

    // Fill header size at first position
    this->CopyData(0, &this->HeaderSize, sizeof(uint16_t));
    return true;
  }
};

//-----------------------------------------------------------------------------
vtkUDPPointSender::vtkUDPPointSender()
  : Internals(new vtkUDPPointSender::vtkInternals())
{
  this->ArraySelections = vtkSmartPointer<vtkDataArraySelection>::New();
  this->ArraySelections->AddObserver(vtkCommand::ModifiedEvent, this, &vtkUDPPointSender::Modified);
}

//-----------------------------------------------------------------------------
vtkUDPPointSender::~vtkUDPPointSender() = default;

//-----------------------------------------------------------------------------
int vtkUDPPointSender::RequestInformation(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!Superclass::GetEnabled())
  {
    return 1;
  }

  if (!Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  auto& internals = *(this->Internals);

  internals.LastValue = std::numeric_limits<double>::min();

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  auto fieldData = input->GetAttributesAsFieldData(vtkDataObject::POINT);
  if (!fieldData)
  {
    return 0;
  }

  if (this->OnlySendNewData)
  {
    if (fieldData->GetAbstractArray(this->TimeArrayName.c_str()))
    {
      if (fieldData->GetAbstractArray(this->TimeArrayName.c_str())->GetNumberOfComponents() != 1)
      {
        vtkErrorMacro("New data array doesn't handle arrays with multiple components.");
        return 0;
      }
    }
    else
    {
      vtkErrorMacro("Must set a valid time array name.");
      return 0;
    }
  }

  // Reset all bytes in buffer
  std::fill(internals.DataBuffer, internals.DataBuffer + ::PACKET_SIZE, 0);

  if (!internals.InitPayloadHeader(fieldData, this->ArraySelections))
  {
    vtkErrorMacro("Could not init header payload.");
    return 0;
  }

  if (internals.MaxPointNb == 0)
  {
    vtkGenericWarningMacro("The point data size is too big, please remove point data arrays.");
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkUDPPointSender::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Pass input to output without any changes
  vtkDataSet* output = vtkDataSet::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(input);

  if (!Superclass::GetEnabled())
  {
    return 1;
  }

  if (!Superclass::OpenSocket())
  {
    return 0;
  }

  this->SendData(input);

  Superclass::CloseSocket();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkUDPPointSender::SendData(vtkDataSet* dataset)
{
  auto& internals = *(this->Internals);

  auto fieldData = dataset->GetAttributesAsFieldData(vtkDataObject::POINT);

  std::fill(internals.DataBuffer + internals.HeaderSize, internals.DataBuffer + ::PACKET_SIZE, 0);
  uint16_t currentIdx = internals.HeaderSize;

  uint16_t nbOfPacketPoint = 0;
  double currentValue = std::numeric_limits<double>::min();
  for (vtkIdType currentPoint = 0; currentPoint < dataset->GetNumberOfPoints(); ++currentPoint)
  {
    if (this->OnlySendNewData)
    {
      auto dataArray =
        vtkDataArray::SafeDownCast(fieldData->GetAbstractArray(this->TimeArrayName.c_str()));
      if (dataArray)
      {
        double value = dataArray->GetComponent(currentPoint, 0);
        if (value > internals.LastValue)
        {
          currentValue = std::max(currentValue, value);
        }
        else
        {
          continue;
        }
      }
    }

    double point[3];
    dataset->GetPoint(currentPoint, point);
    for (uint8_t i = 0; i < 3; ++i)
    {
      float lowerPrecision = static_cast<float>(point[i]);
      currentIdx = internals.CopyData(currentIdx, &lowerPrecision, sizeof(float));
    }
    for (int arrayIdx = 0; arrayIdx < fieldData->GetNumberOfArrays(); ++arrayIdx)
    {
      auto dArray = vtkDataArray::SafeDownCast(fieldData->GetAbstractArray(arrayIdx));
      if (dArray && dArray->GetName() && this->ArraySelections->ArrayIsEnabled(dArray->GetName()))
      {
        for (int compIdx = 0; compIdx < dArray->GetNumberOfComponents(); ++compIdx)
        {
          float lowerPrecision = static_cast<float>(dArray->GetComponent(currentPoint, compIdx));
          currentIdx = internals.CopyData(currentIdx, &lowerPrecision, sizeof(float));
        }
      }
    }

    ++nbOfPacketPoint;
    if ((nbOfPacketPoint == internals.MaxPointNb) ||
      (currentPoint == dataset->GetNumberOfPoints() - 1))
    {
      // Write number of points in header
      const uint16_t nbOfPointOffset = internals.HeaderSize - sizeof(uint8_t) * 4;
      internals.CopyData(nbOfPointOffset, &nbOfPacketPoint, sizeof(uint16_t));

      // Write footer
      const uint16_t footerSize = sizeof(uint16_t) * 2;
      const uint16_t packetRealSize = currentIdx + footerSize;
      currentIdx = internals.CopyData(currentIdx, &packetRealSize, sizeof(uint16_t));
      internals.CopyData(currentIdx, &::PACKET_END, sizeof(uint16_t));

      Superclass::SendData(internals.DataBuffer, ::PACKET_SIZE);

      std::fill(
        internals.DataBuffer + internals.HeaderSize, internals.DataBuffer + ::PACKET_SIZE, 0);
      currentIdx = internals.HeaderSize;
      nbOfPacketPoint = 0;
    }
  }
  internals.LastValue = std::max(currentValue, internals.LastValue);
}

//-----------------------------------------------------------------------------
int vtkUDPPointSender::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkUDPPointSender::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OnlySendNewData: " << this->OnlySendNewData << endl;
  os << indent << "TimeArrayName: " << this->TimeArrayName << endl;
}
