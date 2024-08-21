/*=========================================================================

  Program: LidarView
  Module:  vtkClustersPointsUDPSender.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClustersPointsUDPSender.h"

#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataArrayRange.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkThresholdPoints.h>

#include <algorithm>
#include <limits>
#include <set>

vtkStandardNewMacro(vtkClustersPointsUDPSender);

namespace
{
constexpr uint16_t PACKET_SIZE = 508;
constexpr uint8_t PACKET_START[2] = { 0x28, 0x2a };
constexpr uint8_t HEADER_SIZE = 17;
constexpr uint8_t PACKET_END[2] = { 0x2a, 0x2c };
constexpr uint8_t FOOTER_SIZE = 4U;
constexpr uint8_t POINT_DATA_SIZE = sizeof(float);
constexpr uint16_t MAX_NUMBER_OF_POINTS =
  (::PACKET_SIZE - ::HEADER_SIZE - ::FOOTER_SIZE) / (::POINT_DATA_SIZE * 4);
}

//-----------------------------------------------------------------------------
class vtkClustersPointsUDPSender::vtkInternals
{
public:
  uint8_t DataBuffer[::PACKET_SIZE];

  //-----------------------------------------------------------------------------
  uint16_t CopyData(uint16_t index, const void* data, size_t dataSize)
  {
    return vtkUDPSenderAlgorithm::CopyData(this->DataBuffer, index, data, dataSize);
  }
};

//-----------------------------------------------------------------------------
vtkClustersPointsUDPSender::vtkClustersPointsUDPSender()
  : Internals(new vtkClustersPointsUDPSender::vtkInternals())
{
}

//-----------------------------------------------------------------------------
vtkClustersPointsUDPSender::~vtkClustersPointsUDPSender() = default;

//-----------------------------------------------------------------------------
int vtkClustersPointsUDPSender::RequestInformation(vtkInformation* request,
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
  return 1;
}

//-----------------------------------------------------------------------------
int vtkClustersPointsUDPSender::RequestData(vtkInformation* vtkNotUsed(request),
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

  double requestedTime = 0.0;
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    requestedTime = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  auto& internals = *(this->Internals);

  auto fieldData = input->GetAttributesAsFieldData(vtkDataObject::POINT);
  if (!fieldData)
  {
    return 1;
  }
  auto array = vtkIntArray::SafeDownCast(fieldData->GetAbstractArray("ClusterId"));
  if (!array)
  {
    return 1;
  }
  auto range = vtk::DataArrayValueRange(array);
  std::set<int> clustersIds(range.begin(), range.end());

  if (!Superclass::OpenSocket())
  {
    return 0;
  }

  for (const auto& id : clustersIds)
  {
    // clustersIds equal to -1 should be ignored - considered too small / invalid
    if (id == -1)
    {
      continue;
    }
    vtkSmartPointer<vtkThresholdPoints> selector = vtkSmartPointer<vtkThresholdPoints>::New();
    selector->SetInputData(input);
    selector->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ClusterId");
    selector->ThresholdBetween(id, id);
    selector->Update();
    auto selectedPoints = vtkDataSet::SafeDownCast(selector->GetOutputDataObject(0));
    if (!selectedPoints)
    {
      continue;
    }

    // Reset all bytes in buffer
    std::fill(internals.DataBuffer, internals.DataBuffer + ::PACKET_SIZE, 0);
    this->SendData(selectedPoints, requestedTime, id);
  }

  Superclass::CloseSocket();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkClustersPointsUDPSender::SendData(vtkDataSet* dataset, double timestamp, int clusterId)
{
  auto& internals = *(this->Internals);

  // Clear buffer
  std::fill(internals.DataBuffer, internals.DataBuffer + ::PACKET_SIZE, 0);

  uint16_t currentIdx = 0;
  currentIdx = internals.CopyData(currentIdx, &::PACKET_START, sizeof(uint8_t) * 2);

  const uint8_t dataType = sizeof(float);
  currentIdx = internals.CopyData(currentIdx, &dataType, sizeof(uint8_t));
  currentIdx = internals.CopyData(currentIdx, &timestamp, sizeof(double));

  currentIdx = internals.CopyData(currentIdx, &clusterId, sizeof(int));

  // Skip - placeholder for number of associated array
  const uint16_t numberOfPointsIndex = currentIdx;
  currentIdx += sizeof(uint16_t);

  uint16_t nbOfPacketPoint = 0;
  for (vtkIdType currentPoint = 0; currentPoint < dataset->GetNumberOfPoints(); ++currentPoint)
  {
    double point[3];
    dataset->GetPoint(currentPoint, point);
    for (uint8_t i = 0; i < 3; ++i)
    {
      float lowerPrecision = static_cast<float>(point[i]);
      currentIdx = internals.CopyData(currentIdx, &lowerPrecision, sizeof(float));
    }

    ++nbOfPacketPoint;
    if ((nbOfPacketPoint == ::MAX_NUMBER_OF_POINTS) ||
      (currentPoint == dataset->GetNumberOfPoints() - 1))
    {
      // Write number of points in header
      internals.CopyData(numberOfPointsIndex, &nbOfPacketPoint, sizeof(uint16_t));

      // Write footer
      const uint16_t footerSize = sizeof(uint16_t) * 2;
      const uint16_t packetRealSize = currentIdx + footerSize;
      currentIdx = internals.CopyData(currentIdx, &packetRealSize, sizeof(uint16_t));
      internals.CopyData(currentIdx, &::PACKET_END, sizeof(uint16_t));

      Superclass::SendData(internals.DataBuffer, ::PACKET_SIZE);

      std::fill(internals.DataBuffer + ::HEADER_SIZE, internals.DataBuffer + ::PACKET_SIZE, 0);
      currentIdx = ::HEADER_SIZE;
      nbOfPacketPoint = 0;
    }
  }
}

//-----------------------------------------------------------------------------
int vtkClustersPointsUDPSender::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkClustersPointsUDPSender::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ControlPort: " << this->ControlPort << endl;
}
