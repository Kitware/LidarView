/*=========================================================================

  Program: LidarView
  Module:  vtkPointsPCA.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <numeric> // For std::iota

// Local includes
#include "vtkPointsPCA.h"

// VTK includes
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPCAStatistics.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVector.h>
#include <vtkVectorOperators.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPointsPCA);

//-----------------------------------------------------------------------------
vtkPointsPCA::vtkPointsPCA()
{
  this->PCA = vtkSmartPointer<vtkPCAStatistics>::New();
}

//-----------------------------------------------------------------------------
void vtkPointsPCA::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPointsPCA::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]->GetInformationObject(0));

  auto table = vtkSmartPointer<vtkTable>::New();

  auto xArray = vtkSmartPointer<vtkFloatArray>::New();
  auto yArray = vtkSmartPointer<vtkFloatArray>::New();
  auto zArray = vtkSmartPointer<vtkFloatArray>::New();

  xArray->SetName("X");
  yArray->SetName("Y");
  zArray->SetName("Z");

  // Fill the table with the points from the polyData
  if (this->PointIndices.empty())
  {
    vtkIdType numPoints = input->GetNumberOfPoints();
    this->PointIndices.resize(numPoints);
    std::iota(this->PointIndices.begin(), this->PointIndices.end(), 0);
  }

  for (const auto& ptIdx : this->PointIndices)
  {
    double point[3];
    input->GetPoint(ptIdx, point);
    xArray->InsertNextValue(point[0]);
    yArray->InsertNextValue(point[1]);
    zArray->InsertNextValue(point[2]);
  }

  table->AddColumn(xArray);
  table->AddColumn(yArray);
  table->AddColumn(zArray);

  this->PCA->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, table);
  this->PCA->SetColumnStatus("X", 1);
  this->PCA->SetColumnStatus("Y", 1);
  this->PCA->SetColumnStatus("Z", 1);
  this->PCA->RequestSelectedColumns();

  this->PCA->Update();

  vtkDataSet* output = vtkDataSet::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(input);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPointsPCA::SetPointIndices(const std::vector<int>& pointIndices)
{
  this->PointIndices = pointIndices;
}

//-----------------------------------------------------------------------------
void vtkPointsPCA::GetEigenValues(vtkDoubleArray* array)
{
  this->PCA->GetEigenvalues(array);
}

//-----------------------------------------------------------------------------
vtkVector3d vtkPointsPCA::GetEigenValues()
{
  vtkSmartPointer<vtkDoubleArray> eigenValues = vtkSmartPointer<vtkDoubleArray>::New();
  this->PCA->GetEigenvalues(eigenValues);
  return vtkVector3d(eigenValues->GetValue(0), eigenValues->GetValue(1), eigenValues->GetValue(2));
}

//-----------------------------------------------------------------------------
double vtkPointsPCA::GetEigenValue(int i)
{
  return this->PCA->GetEigenvalue(i);
}

//-----------------------------------------------------------------------------
void vtkPointsPCA::GetEigenVectors(vtkDoubleArray* array)
{
  this->PCA->GetEigenvectors(array);
}

//-----------------------------------------------------------------------------
void vtkPointsPCA::GetEigenVector(int i, vtkDoubleArray* array)
{
  this->PCA->GetEigenvectors(i, array);
}

//-----------------------------------------------------------------------------
vtkVector3d vtkPointsPCA::GetEigenVector(int i)
{
  vtkSmartPointer<vtkDoubleArray> eigenVectors = vtkSmartPointer<vtkDoubleArray>::New();
  this->PCA->GetEigenvectors(eigenVectors);
  double eigenVector[3];
  eigenVectors->GetTuple(i, eigenVector);
  return vtkVector3d(eigenVector[0], eigenVector[1], eigenVector[2]);
}
