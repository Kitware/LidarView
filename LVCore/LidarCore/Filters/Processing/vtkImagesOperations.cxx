/*=========================================================================

  Program: LidarView
  Module:  vtkImagesOperations.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImagesOperations.h"

#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <Eigen/Dense>

vtkStandardNewMacro(vtkImagesOperations);

//-----------------------------------------------------------------------------
vtkImagesOperations::vtkImagesOperations()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
int vtkImagesOperations::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return VTK_OK;
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return VTK_OK;
  }

  return VTK_ERROR;
}

//-----------------------------------------------------------------------------
int vtkImagesOperations::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkImageData* inputImage0 = vtkImageData::GetData(inputVector[0]->GetInformationObject(0));
  vtkImageData* inputImage1 = vtkImageData::GetData(inputVector[1]->GetInformationObject(0));
  vtkImageData* outputImage = vtkImageData::GetData(outputVector, 0);

  int dimensions0[3];
  int dimensions1[3];
  inputImage0->GetDimensions(dimensions0);
  inputImage1->GetDimensions(dimensions1);

  if (dimensions0[0] != dimensions1[0] || dimensions0[1] != dimensions1[1])
  {
    return VTK_ERROR;
  }

  vtkNew<vtkImageData> newImage;
  newImage->SetDimensions(dimensions0);
  newImage->AllocateScalars(VTK_DOUBLE, 1);
  newImage->SetOrigin(0, 0, 0);
  newImage->SetSpacing(.1, .1, 1);

  std::function<double(double, double)> operation;
  switch (this->Operation)
  {
    case Operator::SUM:
      operation = [](double a, double b) { return a + b; };
      break;
    case Operator::DIFFERENCE:
      operation = [](double a, double b) { return a - b; };
      break;
    case Operator::ABSOLUTE_DIFFERENCE:
      operation = [](double a, double b) { return std::abs(a - b); };
      break;
    case Operator::PRODUCT:
      operation = [](double a, double b) { return a * b; };
      break;
    default:
      vtkErrorMacro("The specified operation type doesn't exist");
      return VTK_ERROR;
  }

  for (int y = 0; y < dimensions0[1]; y++)
  {
    for (int x = 0; x < dimensions0[0]; x++)
    {
      double value0 = inputImage0->GetScalarComponentAsDouble(x, y, 0, 0);
      double value1 = inputImage1->GetScalarComponentAsDouble(x, y, 0, 0);

      double newValue = operation(value0, value1);
      newImage->SetScalarComponentFromDouble(x, y, 0, 0, newValue);
    }
  }

  newImage->GetPointData()->GetScalars()->SetName(this->ResultScalarName.c_str());
  outputImage->ShallowCopy(newImage);

  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkImagesOperations::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  int* extent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, 1);
  return VTK_OK;
}
