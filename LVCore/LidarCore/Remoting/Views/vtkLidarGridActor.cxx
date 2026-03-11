/*=========================================================================

  Program: LidarView
  Module:  vtkLidarGridActor.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarGridActor.h"

#include <vtkPolyDataMapper.h>
#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>

#include "vtkGridSource.h"

vtkStandardNewMacro(vtkLidarGridActor);

//----------------------------------------------------------------------------
vtkLidarGridActor::vtkLidarGridActor()
{
  this->Grid = vtkSmartPointer<vtkGridSource>::New();
  this->Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->Mapper->SetInputConnection(this->Grid->GetOutputPort());
  this->SetMapper(this->Mapper);
}

//----------------------------------------------------------------------------
void vtkLidarGridActor::SetGridScale(double value)
{
  this->Grid->SetScale(value);
}

//----------------------------------------------------------------------------
void vtkLidarGridActor::SetGridOrigin(double originX, double originY, double originZ)
{
  this->Grid->SetOrigin(originX, originY, originZ);
}

//----------------------------------------------------------------------------
void vtkLidarGridActor::SetGridNormal(double normalX, double normalY, double normalZ)
{
  this->Grid->SetNormal(normalX, normalY, normalZ);
}

//----------------------------------------------------------------------------
void vtkLidarGridActor::SetGridNumberOfTicks(int value)
{
  this->Grid->SetGridNbTicks(value);
}

//----------------------------------------------------------------------------
void vtkLidarGridActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
