/*=========================================================================

  Program: LidarView
  Module:  vtkSelectPointsByScalars.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Local includes
#include "vtkSelectPointsByScalars.h"

// VTK includes
#include <vtkCellArray.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSelectPointsByScalars);

//-----------------------------------------------------------------------------
void vtkSelectPointsByScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkSelectPointsByScalars::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData* output = vtkPolyData::GetData(outputVector->GetInformationObject(0));

  if (!input)
  {
    vtkErrorMacro(<< "Invalid input!");
    return 1;
  }

  vtkDataArray* inScalars;

  if (!(inScalars = this->GetInputArrayToProcess(0, inputVector)))
  {
    vtkErrorMacro(<< "No scalar data to process");
    return 1;
  }

  if (inScalars->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro(<< "Can only handle one component scalar arrays");
    return 1;
  }

  vtkPointData* inPD;
  inPD = input->GetPointData();

  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD);

  vtkNew<vtkPoints> newPoints;
  vtkNew<vtkCellArray> verts;

  vtkPoints* points = input->GetPoints();

  for (vtkIdType idx = 0; idx < points->GetNumberOfPoints(); idx++)
  {
    double scalarVal = inScalars->GetComponent(idx, 0);

    bool match = false;
    for (int requiredValue : this->ValuesToProcess)
    {
      if (scalarVal == requiredValue)
      {
        match = true;
        break;
      }
    }

    const bool select = this->InvertSelection ? !match : match;
    if (select)
    {
      vtkIdType newPtId = newPoints->InsertNextPoint(points->GetPoint(idx));

      verts->InsertNextCell(1);
      verts->InsertCellPoint(newPtId);

      outPD->CopyData(inPD, idx, newPtId);
    }
  }

  output->SetPoints(newPoints);
  output->SetVerts(verts);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSelectPointsByScalars::SetNumberOfValues(int values)
{
  this->ValuesToProcess.resize(values);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSelectPointsByScalars::SetValue(int index, double value)
{
  this->ValuesToProcess[index] = value;
  this->Modified();
}
