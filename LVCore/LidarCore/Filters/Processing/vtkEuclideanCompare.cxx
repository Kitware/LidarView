/*=========================================================================

  Program: LidarView
  Module:  vtkEuclideanCompare.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkEuclideanCompare.h"

#include <vtkAbstractPointLocator.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkKdTreePointLocator.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointLocator.h>
#include <vtkPoints.h>
#include <vtkSMPTools.h>
#include <vtkSmartPointer.h>
#include <vtkVoxelGridProcessor.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEuclideanCompare)

//----------------------------------------------------------------------------
vtkEuclideanCompare::vtkEuclideanCompare()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkEuclideanCompare::~vtkEuclideanCompare() = default;

//----------------------------------------------------------------------------
int vtkEuclideanCompare::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkEuclideanCompare::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->UseKDTree)
  {
    this->Locator = vtkSmartPointer<vtkKdTreePointLocator>::New();
  }
  else
  {
    this->Locator = vtkSmartPointer<vtkPointLocator>::New();
  }
  // Reset locator mtime to update it as it may have changed.
  this->ReferenceLastMTime = 0;
  return 1;
}

//----------------------------------------------------------------------------
void vtkEuclideanCompare::Compare(vtkPoints* refPoints,
  vtkDataSet* targetDs,
  vtkDoubleArray* outputArr)
{
  vtkPoints* comparePts = targetDs->GetPoints();
  for (vtkIdType idx = 0; idx < targetDs->GetNumberOfPoints(); idx++)
  {
    double point[3];
    comparePts->GetPoint(idx, point);
    vtkIdType closestId = this->Locator->FindClosestPoint(point);
    double refPoint[3];
    refPoints->GetPoint(closestId, refPoint);
    double distance = std::sqrt(std::pow(refPoint[0] - point[0], 2) +
      std::pow(refPoint[1] - point[1], 2) + std::pow(refPoint[2] - point[2], 2));
    outputArr->SetValue(idx, distance);
  }
}

//----------------------------------------------------------------------------
void vtkEuclideanCompare::CompareWithVoxelGrid(vtkPoints* refPoints,
  vtkDataSet* targetDs,
  vtkDoubleArray* outputArr)
{
  vtkNew<vtkVoxelGridProcessor> voxelGrid;
  voxelGrid->SetLeafSize(this->LeafSize);
  voxelGrid->SetSampling(vtkVoxelGridProcessor::FIRST);
  voxelGrid->AddPoints(targetDs);
  auto grid = voxelGrid->GetVoxelGrid();

  std::unordered_map<uint64_t, double> distanceMap;
  distanceMap.reserve(grid.size());
  for (auto& [idx, pointId] : grid)
  {
    double point[3];
    voxelGrid->GetOutput()->GetPoints()->GetPoint(pointId, point);
    vtkIdType closestId = this->Locator->FindClosestPoint(point);
    double refPoint[3];
    refPoints->GetPoint(closestId, refPoint);
    double distance = std::sqrt(std::pow(refPoint[0] - point[0], 2) +
      std::pow(refPoint[1] - point[1], 2) + std::pow(refPoint[2] - point[2], 2));
    distanceMap[idx] = distance;
  }

  vtkPoints* comparePts = targetDs->GetPoints();
  auto fillDistanceArray = [&](vtkIdType begin, vtkIdType end)
  {
    for (vtkIdType idx = begin; idx < end; idx++)
    {
      double point[3];
      comparePts->GetPoint(idx, point);
      uint64_t gridId = voxelGrid->GetPointId(point);
      outputArr->SetValue(idx, distanceMap[gridId]);
    }
  };
  vtkSMPTools::For(0, targetDs->GetNumberOfPoints(), fillDistanceArray);
}

//----------------------------------------------------------------------------
int vtkEuclideanCompare::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataSet* referenceDs = vtkDataSet::GetData(inputVector[0]->GetInformationObject(0));
  vtkDataSet* targetDs = vtkDataSet::GetData(inputVector[1]->GetInformationObject(0));

  if (!referenceDs || !targetDs)
  {
    vtkErrorMacro("Inputs must be two valid vtkDataSet!");
    return 0;
  }

  if (!this->Locator)
  {
    vtkErrorMacro("The point locator is not initialized correctly!");
    return 0;
  }

  vtkDataSet* reference = referenceDs;

  // If reference changed must update the locator
  if (referenceDs->GetMTime() > this->ReferenceLastMTime)
  {
    double refBounds[6];
    referenceDs->GetBounds(refBounds);
    this->Locator->SetDataSet(reference);
    this->Locator->BuildLocator();
    this->ReferenceLastMTime = referenceDs->GetMTime();
  }

  vtkSmartPointer<vtkDoubleArray> distanceArr = vtkSmartPointer<vtkDoubleArray>::New();
  distanceArr->SetNumberOfComponents(1);
  distanceArr->SetNumberOfTuples(targetDs->GetNumberOfPoints());
  distanceArr->SetName(ResultScalarName.c_str());

  if (this->UseVoxelGrid)
  {
    this->CompareWithVoxelGrid(referenceDs->GetPoints(), targetDs, distanceArr);
  }
  else
  {
    this->Compare(referenceDs->GetPoints(), targetDs, distanceArr);
  }

  vtkDataSet* output = vtkDataSet::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(targetDs);
  output->GetPointData()->AddArray(distanceArr);
  output->GetPointData()->SetActiveScalars(this->ResultScalarName.c_str());
  return 1;
}

//----------------------------------------------------------------------------
void vtkEuclideanCompare::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseKDTree: " << this->UseKDTree << "\n";
  os << indent << "ResultScalarName: " << this->ResultScalarName << "\n";
}
