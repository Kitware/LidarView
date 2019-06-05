// Copyright 2013 Velodyne Acoustics, Inc.
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

// .NAME vtkPlaneFitter - class for finding a plane fit to polydata
// .Section Description
//

#ifndef _vtkPlaneFitter_h
#define _vtkPlaneFitter_h

#include <vtkObject.h>

class vtkPointSet;

class VTK_EXPORT vtkPlaneFitter : public vtkObject
{
public:
  static vtkPlaneFitter* New();
  vtkTypeMacro(vtkPlaneFitter, vtkObject);

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  static void PlaneFit(vtkPointSet* pts, double origin[3], double normal[3], double& minDist,
    double& maxDist, double& stdDev, double channelMean[], double channelStdDev[],
    vtkIdType channelNpts[], unsigned int nchannels);

protected:
  vtkPlaneFitter();
  virtual ~vtkPlaneFitter();

private:
  vtkPlaneFitter(const vtkPlaneFitter&);
  void operator=(const vtkPlaneFitter&);
};

#endif
