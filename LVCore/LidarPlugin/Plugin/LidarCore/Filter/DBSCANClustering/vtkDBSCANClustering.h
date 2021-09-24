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


#ifndef VTK_DBSCAN_CLUSTERING_H
#define VTK_DBSCAN_CLUSTERING_H

// LOCAL
#include "DBSCAN.h"

// VTK
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include "LidarCoreModule.h"

class LIDARCORE_EXPORT vtkDBSCANClustering : public vtkPolyDataAlgorithm
{
public:
  static vtkDBSCANClustering *New();
  vtkTypeMacro(vtkDBSCANClustering, vtkPolyDataAlgorithm)

  vtkSetMacro(Epsilon, double);
  vtkSetMacro(MinPts, double);

protected:
  vtkDBSCANClustering() = default;
  ~vtkDBSCANClustering() = default;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:
  vtkDBSCANClustering(const vtkDBSCANClustering&) = delete;
  void operator=(const vtkDBSCANClustering&) = delete;

  double Epsilon = 0.5;
  double MinPts = 5;
};

#endif // VTK_DBSCAN_CLUSTERING_H
