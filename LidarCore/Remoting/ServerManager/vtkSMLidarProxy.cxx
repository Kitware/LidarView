/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarProxy.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMLidarProxy.h"

#include <vtkObjectFactory.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyProperty.h>

#include <cstring>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMLidarProxy);

//----------------------------------------------------------------------------
vtkSMLidarProxy::vtkSMLidarProxy() = default;

//----------------------------------------------------------------------------
vtkSMLidarProxy::~vtkSMLidarProxy() = default;

//----------------------------------------------------------------------------
std::string vtkSMLidarProxy::GetLidarInformation()
{
  auto vendor = vtkSMPropertyHelper(this->GetProperty("Vendor"));
  auto modelName = vtkSMPropertyHelper(this->GetProperty("ModelName"));
  if (!vendor.GetAsString())
  {
    return "";
  }
  std::string info = vendor.GetAsString();
  if (!modelName.GetAsString() || std::strlen(modelName.GetAsString()) == 0)
  {
    return info;
  }
  return info + "-" + modelName.GetAsString();
}

//----------------------------------------------------------------------------
void vtkSMLidarProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
