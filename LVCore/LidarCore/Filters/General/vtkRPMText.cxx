/*=========================================================================

  Program: LidarView
  Module:  vtkRPMText.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRPMText.h"

#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkRPMText);

namespace
{
double SearchForInfoData(vtkDataObject* dataset, std::string fieldName)
{
  double rpm = 0;
  vtkFieldData* fieldData = dataset->GetFieldData();
  if (fieldData)
  {
    vtkDoubleArray* array =
      vtkDoubleArray::SafeDownCast(fieldData->GetAbstractArray(fieldName.c_str()));
    if (array && array->GetNumberOfTuples() == 1)
    {
      array->GetTuple(0, &rpm);
    }
  }
  return rpm;
}
}

//----------------------------------------------------------------------------
vtkRPMText::vtkRPMText()
{
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
int vtkRPMText::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkRPMText::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]->GetInformationObject(0));

  vtkMultiBlockDataSet* multiDataset = vtkMultiBlockDataSet::SafeDownCast(input);
  if (multiDataset)
  {
    if (multiDataset->GetNumberOfBlocks() >= 1)
    {
      input = multiDataset->GetBlock(0);
    }
  }

  const double rpm = ::SearchForInfoData(input, "RPM");
  const double fps = ::SearchForInfoData(input, "FPS");

  std::ostringstream oss;
  std::string text = "No RPM/FPS";
  if (rpm != 0)
  {
    oss << std::setprecision(6) << std::noshowpoint << rpm;
    text = "RPM: " + oss.str();
  }
  else if (fps != 0)
  {
    oss << std::setprecision(6) << std::noshowpoint << fps;
    text = "FPS: " + oss.str();
  }

  vtkTable* output = this->GetOutput();

  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(text.c_str());
  output->AddColumn(data);
  data->Delete();

  return 1;
}
