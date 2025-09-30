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

#include <vtkCharArray.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkLongArray.h>
#include <vtkLongLongArray.h>
#include <vtkShortArray.h>
#include <vtkSignedCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkUnsignedLongLongArray.h>
#include <vtkUnsignedShortArray.h>

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
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return VTK_OK;
  }

  return VTK_ERROR;
}

//-----------------------------------------------------------------------------
template <typename T>
std::function<T(T, T)> vtkImagesOperations::GetOperation()
{
  switch (this->Operation)
  {
    case OperationType::SUM:
      return [](T a, T b) { return a + b; };
    case OperationType::DIFF:
      return [](T a, T b) { return a - b; };
    case OperationType::ABSOLUTE_DIFF:
      // We can't directly return std::abs(a - b) because of unsigned types
      return [](T a, T b) { return (a > b) ? (a - b) : (b - a); };
    case OperationType::PRODUCT:
      return [](T a, T b) { return a * b; };
    default:
      vtkErrorMacro("The specified operation type doesn't exist");
      return [](T a, T vtkNotUsed(b)) { return a; };
  }
};

template <>
std::function<double(double, double)> vtkImagesOperations::GetOperation()
{
  switch (this->Operation)
  {
    case OperationType::SUM:
      return [](double a, double b) { return a + b; };
    case OperationType::DIFF:
      return [](double a, double b) { return a - b; };
    case OperationType::ABSOLUTE_DIFF:
      // We can't directly return std::abs(a - b) because of unsigned types
      return [](double a, double b) { return (a > b) ? (a - b) : (b - a); };
    case OperationType::PRODUCT:
      return [](double a, double b) { return a * b; };
    default:
      vtkErrorMacro("The specified operation type doesn't exist");
      return [](double a, double vtkNotUsed(b)) { return a; };
  }
};

//-----------------------------------------------------------------------------
template <typename ArrayT>
void vtkImagesOperations::PerformImageOperation(vtkAbstractArray* inArray1,
  vtkAbstractArray* inArray2,
  vtkPointData* outPointData)
{
  int numberOfValues = inArray1->GetNumberOfValues();
  ArrayT* _inArray1 = ArrayT::FastDownCast(inArray1);
  ArrayT* _inArray2 = ArrayT::FastDownCast(inArray2);

  vtkSmartPointer<ArrayT> _outArray = vtkSmartPointer<ArrayT>::New();
  _outArray->SetNumberOfValues(numberOfValues);
  _outArray->SetName(this->ResultScalarName.c_str());

  auto operation = GetOperation<typename ArrayT::ValueType>();

  for (int i = 0; i < numberOfValues; i++)
  {
    auto value0 = _inArray1->GetValue(i);
    auto value1 = _inArray2->GetValue(i);

    auto newValue = operation(value0, value1);
    _outArray->SetValue(i, newValue);
  }

  outPointData->AddArray(_outArray);
};

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

  // Check for same image dimensions
  if (dimensions0[0] != dimensions1[0] || dimensions0[1] != dimensions1[1])
  {
    vtkWarningMacro("Images dimensions must match.");
    return VTK_ERROR;
  }

  // Check if the selected scalar name exist on input data
  vtkAbstractArray* arr0 = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (arr0 == nullptr)
  {
    vtkWarningMacro("The specified scalar doesn't exist on first image.");
    return VTK_ERROR;
  }

  // Create the new image as a copy of the first image
  vtkSmartPointer<vtkImageData> newImage = vtkSmartPointer<vtkImageData>::New();
  newImage->DeepCopy(inputImage0);
  vtkPointData* newPointData = newImage->GetPointData();
  vtkPointData* pointData1 = inputImage1->GetPointData();

  // Extract selected scalar array
  const char* scalarName = arr0->GetName();
  vtkAbstractArray* arr1 = pointData1->GetAbstractArray(scalarName);
  if (arr1 == nullptr)
  {
    vtkWarningMacro("The specified scalar doesn't exist on second image.");
    return VTK_ERROR;
  }

  // Check for similar types between both images
  int type0 = arr0->GetDataType();
  int type1 = arr1->GetDataType();
  if (type0 != type1)
  {
    vtkWarningMacro("Scalar types of both images must match");
    return VTK_ERROR;
  }

  int nbComponents0 = arr0->GetNumberOfComponents();
  int nbComponents1 = arr1->GetNumberOfComponents();
  if (nbComponents0 > 1 || nbComponents1 > 1)
  {
    vtkWarningMacro("Currently, only single components scalars are supported");
    return VTK_ERROR;
  }

  // Switch through VTK Types to call the correct template
  // PerformImageOperationVariant(arr0, arr1, newPointData);
  switch (type0)
  {
    case VTK_CHAR:
      PerformImageOperation<vtkCharArray>(arr0, arr1, newPointData);
      break;
    case VTK_SIGNED_CHAR:
      PerformImageOperation<vtkSignedCharArray>(arr0, arr1, newPointData);
      break;
    case VTK_UNSIGNED_CHAR:
      PerformImageOperation<vtkUnsignedCharArray>(arr0, arr1, newPointData);
      break;
    case VTK_SHORT:
      PerformImageOperation<vtkShortArray>(arr0, arr1, newPointData);
      break;
    case VTK_UNSIGNED_SHORT:
      PerformImageOperation<vtkUnsignedShortArray>(arr0, arr1, newPointData);
      break;
    case VTK_INT:
      PerformImageOperation<vtkIntArray>(arr0, arr1, newPointData);
      break;
    case VTK_UNSIGNED_INT:
      PerformImageOperation<vtkUnsignedIntArray>(arr0, arr1, newPointData);
      break;
    case VTK_LONG:
      PerformImageOperation<vtkLongArray>(arr0, arr1, newPointData);
      break;
    case VTK_UNSIGNED_LONG:
      PerformImageOperation<vtkUnsignedLongArray>(arr0, arr1, newPointData);
      break;
    case VTK_LONG_LONG:
      PerformImageOperation<vtkLongLongArray>(arr0, arr1, newPointData);
      break;
    case VTK_UNSIGNED_LONG_LONG:
      PerformImageOperation<vtkUnsignedLongLongArray>(arr0, arr1, newPointData);
      break;
    case VTK_FLOAT:
      PerformImageOperation<vtkFloatArray>(arr0, arr1, newPointData);
      break;
    case VTK_DOUBLE:
      PerformImageOperation<vtkDoubleArray>(arr0, arr1, newPointData);
      break;
    default:
      vtkWarningMacro("Unsupported scalar type.");
      break;
  }

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
