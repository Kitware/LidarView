//=========================================================================
//
// Copyright 2020 Kitware, Inc.
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

#include "vtkDBSCANClustering.h"
#include "vtkHelper.h"
#include "DBSCAN.h"

// VTK
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPolyData.h>

#include <vtkPointData.h>
#include <vtkIntArray.h>
#include <vtkSmartPointer.h>

// STD
#include <iostream>
#include <vector>

// Implementation of the New function
vtkStandardNewMacro(vtkDBSCANClustering)


using vector_of_vectors = std::vector<std::vector<double>>;
namespace
{
vector_of_vectors PointCloudToStdVector(vtkSmartPointer<vtkPolyData> cloud)
{
  vector_of_vectors points;
  points.reserve(cloud->GetNumberOfPoints());
  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    double* p = cloud->GetPoint(pointIdx);
    std::vector<double> apoint = {p[0], p[1], p[2]};
    points.push_back(apoint);
  }
  return points;
}
}

//-----------------------------------------------------------------------------
int vtkDBSCANClustering::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  auto dbscan = DBSCAN<double>{this->Epsilon, this->MinPts};

  // IO
  vtkPolyData* pointCloud = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData* outCloud = vtkPolyData::GetData(outputVector->GetInformationObject(0));

  outCloud->DeepCopy(pointCloud);
  std::vector<int> pointLabels = dbscan.fit(PointCloudToStdVector(outCloud));

  auto clusterArray = createArray<vtkIntArray>("cluster", 1, outCloud->GetNumberOfPoints());
  outCloud->GetPointData()->AddArray(clusterArray);
  clusterArray->Fill(-1);

  for (unsigned int pointIdx = 0; pointIdx < outCloud->GetNumberOfPoints(); ++pointIdx)
  {
    clusterArray->SetTuple1(pointIdx, pointLabels[pointIdx]);
  }

  return 1;
}