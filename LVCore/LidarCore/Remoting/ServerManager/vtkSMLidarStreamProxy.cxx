/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarStreamProxy.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMLidarStreamProxy.h"

#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMLidarStreamProxy);

//----------------------------------------------------------------------------
vtkSMLidarStreamProxy::vtkSMLidarStreamProxy() = default;

//----------------------------------------------------------------------------
vtkSMLidarStreamProxy::~vtkSMLidarStreamProxy() = default;

//----------------------------------------------------------------------------
void vtkSMLidarStreamProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
