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

#ifndef VTK_SEPARATE_CLOUD_KNN_H
#define VTK_SEPARATE_CLOUD_KNN_H

// VTK
#include <vtkPolyDataAlgorithm.h>

// EIGEN
#include <Eigen/Dense>



/**
  * @brief  SeparateCloudKnn enables smoothing the values of a specific array of
  * an input point cloud (InputCloud) using the k-nearest neighbors from another
  * reference point cloud(NeighborsCloud)
  * Example usage: Using aggregated points cloud on a time window in order to
  * smooth values over time
  */
class VTK_EXPORT vtkSeparateCloudKnn : public vtkPolyDataAlgorithm
{
public:
  static vtkSeparateCloudKnn *New();
  vtkTypeMacro(vtkSeparateCloudKnn, vtkPolyDataAlgorithm)

  vtkSetMacro(NbNeighbors, int)
  vtkGetMacro(NbNeighbors, int)

  vtkSetMacro(ArrayName, std::string)
  vtkGetMacro(ArrayName, std::string)

  vtkSetMacro(MaxDistance, double)
  vtkGetMacro(MaxDistance, double)


protected:
  vtkSeparateCloudKnn();

  int FillInputPortInformation(int port, vtkInformation *info) override;
  int FillOutputPortInformation(int port, vtkInformation *info) override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:
  vtkSeparateCloudKnn(const vtkSeparateCloudKnn&) = delete;
  void operator=(const vtkSeparateCloudKnn&) = delete;

  //! Number of neighbors to consider
  int NbNeighbors = 3;

  //! Array to smooth using nearest neighbors
  std::string ArrayName = "category";

  //! Maximum distance for neighbors to be considered (in meters)
  double MaxDistance = std::numeric_limits<double>::max();
};

#endif // VTK_SEPARATE_CLOUD_KNN_H
