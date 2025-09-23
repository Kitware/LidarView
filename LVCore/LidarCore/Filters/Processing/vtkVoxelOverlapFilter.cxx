/*=========================================================================

  Program: LidarView
  Module:  vtkVoxelOverlapFilter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVoxelOverlapFilter.h"

#include <vtkDataObject.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSMPTools.h>
#include <vtkShortArray.h>
#include <vtkSmartPointer.h>
#include <vtkVoxelGridProcessor.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVoxelOverlapFilter)

//----------------------------------------------------------------------------
vtkVoxelOverlapFilter::vtkVoxelOverlapFilter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkVoxelOverlapFilter::~vtkVoxelOverlapFilter() = default;

//----------------------------------------------------------------------------
int vtkVoxelOverlapFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkVoxelOverlapFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* referenceDs = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData* targetDs = vtkPolyData::GetData(inputVector[1]->GetInformationObject(0));

  if (!referenceDs || !targetDs)
  {
    vtkErrorMacro("Inputs must be two valid vtkPolyData!");
    return 0;
  }

  vtkNew<vtkVoxelGridProcessor> voxelGrid;
  voxelGrid->SetLeafSize(this->LeafSize);
  voxelGrid->SetSampling(vtkVoxelGridProcessor::FIRST);

  double refBounds[6];
  referenceDs->GetBounds(refBounds);
  double compareBounds[6];
  targetDs->GetBounds(compareBounds);
  double voxelGridBounds[6];
  for (unsigned int idx = 0; idx < 3; idx++)
  {
    voxelGridBounds[idx * 2] = std::min(refBounds[idx * 2], compareBounds[idx * 2]);
    voxelGridBounds[idx * 2 + 1] = std::max(refBounds[idx * 2 + 1], compareBounds[idx * 2 + 1]);
  }
  voxelGrid->SetBounds(voxelGridBounds);
  voxelGrid->AddPoints(referenceDs);

  vtkSmartPointer<vtkShortArray> distanceArr = vtkSmartPointer<vtkShortArray>::New();
  distanceArr->SetNumberOfComponents(1);
  distanceArr->SetNumberOfTuples(targetDs->GetNumberOfPoints());
  distanceArr->SetName(this->ResultScalarName.c_str());

  vtkPoints* comparePts = targetDs->GetPoints();
  for (int idx = 0; idx < comparePts->GetNumberOfPoints(); ++idx)
  {
    double point[3];
    comparePts->GetPoint(idx, point);
    bool hasPoint = voxelGrid->HasPointInVoxel(point);
    distanceArr->SetValue(idx, hasPoint);
  }

  vtkPolyData* output = vtkPolyData::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(targetDs);
  output->GetPointData()->AddArray(distanceArr);
  output->GetPointData()->SetActiveScalars(this->ResultScalarName.c_str());

  return 1;
}

//----------------------------------------------------------------------------
void vtkVoxelOverlapFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LeafSize: " << this->LeafSize << "\n";
  os << indent << "ResultScalarName: " << this->ResultScalarName << "\n";
}
