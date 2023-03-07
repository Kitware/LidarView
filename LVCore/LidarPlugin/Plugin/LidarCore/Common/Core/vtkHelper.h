//=========================================================================
//
// Copyright 2019 Kitware, Inc.
// Author: Guilbert Pierre (pierre.guilbert@kitware.com)
// Data: 03-27-2019
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

#ifndef VTK_HELPER_H
#define VTK_HELPER_H

// STD
#include <string>
// VTK
#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>


template <typename T>
vtkSmartPointer<T> createArray(const std::string& Name, int NumberOfComponents = 1, int NumberOfTuples = 0)
{
  vtkSmartPointer<T> array = vtkSmartPointer<T>::New();
  array->SetNumberOfComponents(NumberOfComponents);
  array->SetNumberOfTuples(NumberOfTuples);
  array->SetName(Name.c_str());
  return array;
}

template <typename arrayT, typename T>
vtkSmartPointer<arrayT> addArrayWithDefault(const char* name, vtkPolyData* pd, T defaultValue)
{
  auto array = createArray<arrayT>(name, 1, pd->GetNumberOfPoints());
  array->Fill(defaultValue);
  pd->GetPointData()->AddArray(array);
  return array;
}

// returns position of the closest element to x in v
// if v is empty, returns -1
template <typename T>
int closestElementInOrderedVector(const std::vector<T>& v, T x)
{
  if (v.size() == 0)
  {
    return -1;
  }

  auto lb = std::lower_bound(v.begin(), v.end(), x);
  int n = std::distance(v.begin(), lb);

  if (n == static_cast<int>(v.size()))
  {
    return n - 1;
  }
  else if (n == 0)
  {
      return n;
  }
  else
  {
    // We check if (n - 1) gives a time closer than n gives.
    // We now that  v[n - 1] < x <= v[n]
    if (x - v[n - 1] < v[n] - x)
    {
      return n - 1;
    }
    else
    {
      return n;
    }
  }
}

vtkSmartPointer<vtkPolyLine> CreatePolyLineFromPoints(const vtkSmartPointer<vtkPoints> & points);

#endif // VTK_HELPER_H
