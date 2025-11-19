// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExternalSensorReader.h"
#include "vtkNormalizeExternalSensorData.h"

#include <vtkDelimitedTextReader.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkExternalSensorReader);
vtkCxxSetObjectMacro(vtkExternalSensorReader, Normalizer, vtkNormalizeExternalSensorData);

//------------------------------------------------------------------------------
vtkExternalSensorReader::vtkExternalSensorReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkExternalSensorReader::~vtkExternalSensorReader()
{
  this->SetNormalizer(nullptr);
}

//------------------------------------------------------------------------------
int vtkExternalSensorReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
}
//------------------------------------------------------------------------------
vtkStringArray* vtkExternalSensorReader::GetHeaderColumns()
{
  // Make sure header columns are available for first-time UI queries.
  this->ParseHeaderIfNeeded();

  // Returns a snapshot of the parsed CSV header as a string array
  // so the UI can populate dropdowns.
  if (!this->HeaderColumnsCacheArray)
  {
    this->HeaderColumnsCacheArray = vtkSmartPointer<vtkStringArray>::New();
  }
  vtkStringArray* arr = this->HeaderColumnsCacheArray;
  arr->Initialize();
  for (const auto& name : this->HeaderColumns)
  {
    arr->InsertNextValue(name);
  }
  return arr;
}

//------------------------------------------------------------------------------
int vtkExternalSensorReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkTable* output = vtkTable::GetData(outputVector, 0);
  output->Initialize();

  if (this->FileName.empty() || !vtksys::SystemTools::FileExists(this->FileName))
  {
    vtkErrorMacro(<< "Invalid CSV file: " << (this->FileName.empty() ? "<null>" : this->FileName));
    return 0;
  }

  vtkNew<vtkDelimitedTextReader> csv;
  csv->SetFileName(this->FileName.c_str());
  csv->SetFieldDelimiterCharacters(",");
  csv->SetHaveHeaders(true);
  csv->SetDetectNumericColumns(true);
  csv->SetForceDouble(true);
  csv->Update();

  vtkTable* inTable = csv->GetOutput();
  if (!inTable || inTable->GetNumberOfRows() == 0)
  {
    vtkErrorMacro(<< "Empty or unreadable CSV: " << this->FileName);
    return 0;
  }

  if (!this->Normalizer)
  {
    vtkErrorMacro(
      << "Normalizer subproxy is not set. Please ensure XML defines a SubProxy 'SetNormalizer'.");
    output->ShallowCopy(inTable);
    return 1;
  }

  this->Normalizer->SetInputData(inTable);
  this->Normalizer->Update();
  output->ShallowCopy(this->Normalizer->GetOutput());
  return 1;
}

//------------------------------------------------------------------------------
int vtkExternalSensorReader::RequestInformation(vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outVec)
{
  vtkInformation* outInfo = outVec->GetInformationObject(0);
  outInfo->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");

  this->LoadHeaderFromFile();

  return 1;
}

//------------------------------------------------------------------------------
// Include transform modification times so pipeline updates when calibration changes.
vtkMTimeType vtkExternalSensorReader::GetMTime()
{
  vtkMTimeType mt = this->Superclass::GetMTime();
  if (this->Normalizer)
  {
    mt = std::max(mt, this->Normalizer->GetMTime());
  }
  return mt;
}

//------------------------------------------------------------------------------
void vtkExternalSensorReader::ParseHeaderIfNeeded()
{
  if (!this->HeaderColumns.empty())
  {
    return;
  }
  this->LoadHeaderFromFile();
}

//------------------------------------------------------------------------------
void vtkExternalSensorReader::LoadHeaderFromFile()
{
  this->HeaderColumns.clear();
  if (this->FileName.empty() || !vtksys::SystemTools::FileExists(this->FileName))
  {
    return;
  }

  vtkNew<vtkDelimitedTextReader> csv;
  csv->SetFileName(this->FileName.c_str());
  csv->SetFieldDelimiterCharacters(",");
  csv->SetHaveHeaders(true);
  csv->SetDetectNumericColumns(false);
  csv->SetForceDouble(false);
  // Only parse headers to avoid loading full CSV in information pass
  csv->SetMaxRecords(0);
  csv->Update();

  vtkTable* table = csv->GetOutput();
  if (!table)
  {
    return;
  }

  const int ncols = table->GetNumberOfColumns();
  for (int i = 0; i < ncols; ++i)
  {
    vtkAbstractArray* aa = table->GetColumn(i);
    const char* name = aa ? aa->GetName() : nullptr;
    if (name && *name)
    {
      this->HeaderColumns.emplace_back(name);
    }
  }
}
