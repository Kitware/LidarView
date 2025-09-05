/*=========================================================================

  Program:   LidarView
  Module:    vtkTemporalTransformApplier.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAssociateTableToTemporalDataset.h"

#include <vtkAbstractArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkShortArray.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTable.h>

#include "vtkHelper.h"
#include "vtkPipelineTools.h"

#include <optional>

namespace
{
constexpr int DATASET_INPUT_PORT = 0;
constexpr int TABLE_INPUT_PORT = 1;
constexpr const char* VALIDITY_ARRAY_NAME = "hasAssociatedValues";
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAssociateTableToTemporalDataset)

//-----------------------------------------------------------------------------
vtkAssociateTableToTemporalDataset::vtkAssociateTableToTemporalDataset()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkAssociateTableToTemporalDataset::~vtkAssociateTableToTemporalDataset() = default;

//-----------------------------------------------------------------------------
int vtkAssociateTableToTemporalDataset::FillInputPortInformation(int port, vtkInformation* info)
{

  if (port == ::DATASET_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
    return 1;
  }
  if (port == ::TABLE_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkAssociateTableToTemporalDataset::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataSet* dataset =
    vtkDataSet::GetData(inputVector[::DATASET_INPUT_PORT]->GetInformationObject(0));
  vtkTable* table = vtkTable::GetData(inputVector[::TABLE_INPUT_PORT]->GetInformationObject(0));

  vtkInformation* info = outputVector->GetInformationObject(0);
  if (!info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    vtkErrorMacro(<< "No timestep available!");
    return 0;
  }
  double currentTimestep = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  std::vector<double> allTimesteps = getTimeSteps(info);
  if (allTimesteps.empty())
  {
    vtkErrorMacro(<< "Could not retrieve timesteps informations.");
    return 0;
  }

  unsigned int timestepIdx = closestElementInOrderedVector(allTimesteps, currentTimestep);
  double upperTimestepLimit = std::numeric_limits<double>::max();
  if (timestepIdx + 1 < allTimesteps.size())
  {
    upperTimestepLimit = allTimesteps[timestepIdx + 1];
  }

  vtkDoubleArray* timeArray =
    vtkDoubleArray::SafeDownCast(table->GetColumnByName(this->TimeArrayName.c_str()));
  std::optional<int> lastIndex;
  if (this->AlwaysAssociate)
  {
    // If no value is found takes the last one
    lastIndex = timeArray->GetNumberOfValues() - 1;
  }
  // Iterate over all available timesteps to get the last one in the time range
  for (unsigned int idx = 0; idx < timeArray->GetNumberOfValues(); idx++)
  {
    double time = timeArray->GetValue(idx);
    bool lowerLimitCondition = this->AlwaysAssociate || currentTimestep <= time;
    if (lowerLimitCondition && time < upperTimestepLimit)
    {
      lastIndex = idx;
    }
  }
  vtkSmartPointer<vtkShortArray> validityArray = vtkSmartPointer<vtkShortArray>::New();
  validityArray->SetName(::VALIDITY_ARRAY_NAME);
  validityArray->SetNumberOfComponents(1);
  validityArray->InsertNextValue(lastIndex.has_value());

  vtkFieldData* fieldData = dataset->GetFieldData();
  fieldData->AddArray(validityArray);

  if (lastIndex.has_value())
  {
    // Iterate over all available columns to add the row data in field data
    for (unsigned int idx = 0; idx < table->GetNumberOfColumns(); idx++)
    {
      vtkAbstractArray* array = table->GetColumn(idx);
      vtkAbstractArray* fieldDataArray = fieldData->GetAbstractArray(array->GetName());
      if (fieldDataArray)
      {
        fieldDataArray->InsertNextTuple(lastIndex.value(), array);
      }
      else
      {
        vtkSmartPointer<vtkAbstractArray> newArray;
        newArray.TakeReference(vtkAbstractArray::CreateArray(array->GetDataType()));
        newArray->SetName(array->GetName());
        newArray->InsertNextTuple(lastIndex.value(), array);
        fieldData->AddArray(newArray);
      }
    }
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->ShallowCopy(dataset);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAssociateTableToTemporalDataset::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkTable* table = vtkTable::GetData(inputVector[::TABLE_INPUT_PORT]->GetInformationObject(0));
  if (!table)
  {
    vtkErrorMacro("No valid input table.");
    return 0;
  }

  vtkDoubleArray* timeArray =
    vtkDoubleArray::SafeDownCast(table->GetColumnByName(this->TimeArrayName.c_str()));
  if (!timeArray || timeArray->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro(
      "The table doesn't have a valid column with the " << this->TimeArrayName << " name.");
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
const char* vtkAssociateTableToTemporalDataset::VALIDITY_ARRAY_NAME()
{
  return ::VALIDITY_ARRAY_NAME;
}
