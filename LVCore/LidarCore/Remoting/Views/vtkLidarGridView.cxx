/*=========================================================================

  Program: LidarView
  Module:  vtkLidarGridView.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarGridView.h"

#include <vtkActor.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

#include "vtkLidarGridActor.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarGridView);

//----------------------------------------------------------------------------
vtkLidarGridView::vtkLidarGridView()
{
  this->SetCenterAxesVisibility(0);
}

//----------------------------------------------------------------------------
void vtkLidarGridView::SetLidarGridActor(vtkLidarGridActor* gridActor)
{
  if (this->LidarGridActor != gridActor)
  {
    if (this->LidarGridActor)
    {
      this->GetRenderer()->RemoveActor(this->LidarGridActor);
    }
    this->LidarGridActor = gridActor;
    if (this->LidarGridActor)
    {
      this->GetRenderer()->AddActor(this->LidarGridActor);
    }
  }
}

//----------------------------------------------------------------------------
void vtkLidarGridView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
