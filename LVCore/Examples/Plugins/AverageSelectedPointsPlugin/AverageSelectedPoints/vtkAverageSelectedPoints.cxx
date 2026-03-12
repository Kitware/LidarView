/*=========================================================================

  Program: LidarView
  Module:  vtkAverageSelectedPoints.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAverageSelectedPoints.h"

#include <vtkAbstractArray.h>
#include <vtkDataObject.h>
#include <vtkDelimitedTextReader.h>
#include <vtkImplicitFunction.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSetGet.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

#include <algorithm>
#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAverageSelectedPoints)
vtkCxxSetObjectMacro(vtkAverageSelectedPoints, ImplicitFunction, vtkImplicitFunction);

//----------------------------------------------------------------------------
vtkAverageSelectedPoints::vtkAverageSelectedPoints() = default;

//----------------------------------------------------------------------------
vtkAverageSelectedPoints::~vtkAverageSelectedPoints()
{
  this->SetImplicitFunction(nullptr);
}

//----------------------------------------------------------------------------
int vtkAverageSelectedPoints::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
vtkMTimeType vtkAverageSelectedPoints::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkAverageSelectedPoints::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* inPts = input->GetPoints();

  if (!this->ImplicitFunction)
  {
    vtkErrorMacro(<< "No implicit function specified");
    return 1;
  }

  unsigned int nbOfPts = 0;
  double zPosAverage = 0.;
  for (vtkIdType ptId = 0; ptId < inPts->GetNumberOfPoints(); ptId++)
  {
    double point[3];
    inPts->GetPoint(ptId, point);
    // Signed distance function that return negative value if point in implicit function.
    if (this->ImplicitFunction->FunctionValue(point) <= 0.)
    {
      zPosAverage += point[2];
      nbOfPts++;
    }
  }
  if (nbOfPts)
  {
    zPosAverage /= nbOfPts;
  }

  vtkTable* output = this->GetOutput();

  std::stringstream ss;
  ss << "Average z height: " << std::setprecision(4) << zPosAverage;

  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(ss.str());
  output->AddColumn(data);
  data->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkAverageSelectedPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
