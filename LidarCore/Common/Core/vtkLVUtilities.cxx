/*=========================================================================

  Program:   LidarView
  Module:    vtkLVUtilities.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLVUtilities.h"

#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h> // for vtkStandardNewMacro
#include <vtkTransform.h>

//----------------------------------------------------------------------------
// Implementation of the New function
vtkStandardNewMacro(vtkLVUtilities);

//------------------------------------------------------------------------------
bool vtkLVUtilities::SetTransformInFieldData(vtkFieldData* out,
  vtkTransform* transform,
  const std::string& name)
{
  if (!out || !transform || name.empty())
  {
    return false;
  }
  vtkNew<vtkDoubleArray> arr;
  arr->SetName(name.c_str());
  arr->SetNumberOfComponents(4);
  arr->SetNumberOfTuples(4);
  vtkMatrix4x4* matrix = transform->GetMatrix();
  for (int r = 0; r < 4; ++r)
  {
    for (int c = 0; c < 4; ++c)
    {
      arr->SetComponent(r, c, matrix->GetElement(r, c));
    }
  }
  if (out && !out->HasArray(name.c_str()))
  {
    out->AddArray(arr);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkLVUtilities::GetTransformFromFieldData(vtkFieldData* in,
  vtkMatrix4x4* matrix,
  const std::string& name)
{
  if (!in || !matrix || name.empty())
  {
    return false;
  }

  vtkDoubleArray* arr = vtkDoubleArray::SafeDownCast(in->GetAbstractArray(name.c_str()));
  if (!arr || arr->GetNumberOfComponents() != 4 || arr->GetNumberOfTuples() != 4)
  {
    return false;
  }

  for (int r = 0; r < 4; ++r)
  {
    for (int c = 0; c < 4; ++c)
    {
      matrix->SetElement(r, c, arr->GetComponent(r, c));
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkLVUtilities::GetTransformFromFieldData(vtkFieldData* in,
  vtkTransform* transform,
  const std::string& name)
{
  if (!transform)
  {
    return false;
  }

  vtkNew<vtkMatrix4x4> matrix;
  if (vtkLVUtilities::GetTransformFromFieldData(in, matrix, name))
  {
    transform->SetMatrix(matrix);
    return true;
  }
  return false;
}
