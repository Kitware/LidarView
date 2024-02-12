/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarReaderProxy.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMLidarReaderProxy.h"

#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMLidarReaderProxy);

//----------------------------------------------------------------------------
vtkSMLidarReaderProxy::vtkSMLidarReaderProxy() = default;

//----------------------------------------------------------------------------
vtkSMLidarReaderProxy::~vtkSMLidarReaderProxy() = default;

//----------------------------------------------------------------------------
void vtkSMLidarReaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
