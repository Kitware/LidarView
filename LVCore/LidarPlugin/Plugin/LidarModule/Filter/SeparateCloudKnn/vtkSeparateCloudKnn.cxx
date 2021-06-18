//=========================================================================
// Copyright 2019 Kitware, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

// LOCAL
#include "vtkSeparateCloudKnn.h"
#include "vtkHelper.h"

// VTK
#include <vtkDataArray.h>
#include <vtkIntArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include "KDTreeVectorOfVectorsAdaptor.h"



// Implementation of the New function
vtkStandardNewMacro(vtkSeparateCloudKnn)

//-----------------------------------------------------------------------------
vtkSeparateCloudKnn::vtkSeparateCloudKnn()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
int vtkSeparateCloudKnn::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSeparateCloudKnn::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSeparateCloudKnn::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
// Get inputs
  vtkPolyData *inPointCloud = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData *neighborsPointCloud = vtkPolyData::GetData(inputVector[1]->GetInformationObject(0));

  // Get the output
  vtkPolyData* outPointCloud = vtkPolyData::GetData(outputVector->GetInformationObject(0));

  outPointCloud->DeepCopy(inPointCloud);

  // Prepare KD-tree
  std::vector<std::vector<double>> points;
  std::vector<int> idxArray;
  for (unsigned int pointIdx = 0; pointIdx < neighborsPointCloud->GetNumberOfPoints(); ++pointIdx)
  {
    double* p = neighborsPointCloud->GetPoint(pointIdx);
    std::vector<double> apoint = {p[0], p[1], p[2]};
    points.push_back(apoint);
    idxArray.push_back(pointIdx);
  }

  typedef std::vector<std::vector<double> > my_vector_of_vectors_t;
  typedef KDTreeVectorOfVectorsAdaptor< my_vector_of_vectors_t, double > my_kd_tree_t;

  my_kd_tree_t mat_index(-1 /*dim*/, points, 10 /* max leaf */ );
  mat_index.index->buildIndex();

  // Apply nearest neighbors
  const char * arrayName = this->ArrayName.c_str();
  vtkDataArray* arrayToUpdate = outPointCloud->GetPointData()->GetArray(arrayName);
  vtkDataArray* candidateUpdateValues = neighborsPointCloud->GetPointData()->GetArray(arrayName);

  for (unsigned int pointIdx = 0; pointIdx < outPointCloud->GetNumberOfPoints(); ++pointIdx)
  {
    std::vector<size_t> nearestIndex(this->NbNeighbors, -1);
    std::vector<double> nearestDist(this->NbNeighbors, -1.0);
    std::vector<size_t> prunedNearestIndex;

    double* p = outPointCloud->GetPoint(pointIdx);
    std::vector<double> apoint = {p[0], p[1], p[2]};
    mat_index.query(apoint.data(), this->NbNeighbors, nearestIndex.data(), nearestDist.data());

    // get indices of neighbors
    for (int i = 0; i < this->NbNeighbors; ++i)
    {
      if ( (this->MaxDistance > 0.0) && (nearestDist[i] <= this->MaxDistance)) {

        prunedNearestIndex.push_back(nearestIndex[i]);
      }
    }

    // Update relevant array
    std::vector<int> nearestValues;
    for (std::size_t i = 0; i<prunedNearestIndex.size(); ++i)
    {
      int val = candidateUpdateValues->GetTuple1(prunedNearestIndex[i]);
      if (val != 0)
      {
        nearestValues.push_back(candidateUpdateValues->GetTuple1(prunedNearestIndex[i]));
      }
    }
    int mostCommonValue = mostCommon(nearestValues.begin(), nearestValues.end());
    arrayToUpdate->SetTuple1(pointIdx, mostCommonValue);
  }

  return 1;
}