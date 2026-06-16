/*=========================================================================

  Program: LidarView
  Module:  vtkThresholdFromFile.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkThresholdFromFile.h"

#include <vtkAbstractArray.h>
#include <vtkCellArray.h>
#include <vtkDataObject.h>
#include <vtkDelimitedTextReader.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkTable.h>

#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkThresholdFromFile)

//----------------------------------------------------------------------------
vtkThresholdFromFile::vtkThresholdFromFile() = default;

//----------------------------------------------------------------------------
vtkThresholdFromFile::~vtkThresholdFromFile() = default;

//----------------------------------------------------------------------------
int vtkThresholdFromFile::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check if the array select exist in the input
  vtkAbstractArray* array = input->GetPointData()->GetAbstractArray(this->ActiveScalar.c_str());
  if (!array)
  {
    return 0;
  }

  if (this->FileName.empty())
  {
    return 1;
  }

  // Read csv file
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(this->FileName.c_str());
  reader->SetFieldDelimiterCharacters(",");
  reader->SetStringDelimiter('"');
  reader->Update();

  vtkTable* csvTable = reader->GetOutput();
  if (!csvTable)
  {
    return 0;
  }

  vtkIdType nRows = csvTable->GetNumberOfRows();
  this->FilterValues.reserve(nRows);
  for (vtkIdType idx = 0; idx < nRows; idx++)
  {
    this->FilterValues.emplace_back(csvTable->GetValue(idx, 0).ToDouble());
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkThresholdFromFile::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* inScalars = vtkArrayDownCast<vtkDataArray>(
    input->GetPointData()->GetAbstractArray(this->ActiveScalar.c_str()));
  if (!inScalars)
  {
    return 0;
  }

  vtkIdType numPoints = input->GetNumberOfPoints();
  if (numPoints == 0)
  {
    vtkErrorMacro(<< "No points to threshold");
    return 1;
  }

  vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
  newPoints->Allocate(numPoints);

  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->Allocate(numPoints);

  vtkPointData* pd = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(pd);

  // Filtering is happening here, we iterate over each point and keep it based on file values.
  for (vtkIdType ptId = 0; ptId < numPoints; ptId++)
  {
    double scalarValue = inScalars->GetComponent(ptId, 0);
    auto findWorker = [&](const double& val)
    { return val - this->Tolerance <= scalarValue && scalarValue <= val + this->Tolerance; };
    auto found = std::find_if(this->FilterValues.cbegin(), this->FilterValues.cend(), findWorker);

    bool isFound = found != this->FilterValues.cend();
    if (this->InvertResult)
    {
      isFound = !isFound;
    }

    if (isFound)
    {
      double point[3];
      input->GetPoint(ptId, point);
      vtkIdType pts = newPoints->InsertNextPoint(point);
      outPD->CopyData(pd, ptId, pts);
      verts->InsertNextCell(1, &pts);
    }
  }

  output->SetPoints(newPoints);
  output->SetVerts(verts);

  // Free unused memory
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void vtkThresholdFromFile::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
