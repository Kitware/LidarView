/*=========================================================================

  Program: LidarView
  Module:  vtkGridSource.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGridSource.h"

#include <vtkAppendPolyData.h>
#include <vtkArcSource.h>
#include <vtkExtractEdges.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGridSource)

//-----------------------------------------------------------------------------
vtkGridSource::vtkGridSource()
{
  this->GridNbTicks = 10;
  this->Scale = 10.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkGridSource::~vtkGridSource() {}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkGridSource::CreateGrid(int gridNbTicks,
  double scale,
  double origin[3],
  double normal[3])
{
  vtkNew<vtkPlaneSource> plane;
  vtkNew<vtkExtractEdges> edges;
  vtkNew<vtkAppendPolyData> append;

  plane->SetOrigin(-gridNbTicks * scale, -gridNbTicks * scale, 0.0);
  plane->SetPoint1(gridNbTicks * scale, -gridNbTicks * scale, 0.0);
  plane->SetPoint2(-gridNbTicks * scale, gridNbTicks * scale, 0.0);
  plane->SetResolution(gridNbTicks * 2, gridNbTicks * 2);
  plane->SetCenter(origin);
  plane->SetNormal(normal);

  edges->SetInputConnection(plane->GetOutputPort());
  append->AddInputConnection(edges->GetOutputPort());

  double arcStartVector[3];
  vtkMath::Perpendiculars(normal, arcStartVector, NULL, 0);

  for (int i = 1; i <= gridNbTicks; ++i)
  {
    double startPoint[3] = {
      arcStartVector[0] * i * scale, arcStartVector[1] * i * scale, arcStartVector[2] * i * scale
    };
    vtkNew<vtkArcSource> arc;
    arc->UseNormalAndAngleOn();
    arc->SetCenter(origin);
    arc->SetPolarVector(startPoint);
    arc->SetAngle(360);
    arc->SetNormal(normal);
    arc->SetResolution(360);
    append->AddInputConnection(arc->GetOutputPort());
  }

  append->Update();
  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->ShallowCopy(append->GetOutput());
  return polyData;
}

//-----------------------------------------------------------------------------
int vtkGridSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  if (this->GridNbTicks < 1)
  {
    vtkErrorMacro(
      "Specified grid size " << this->GridNbTicks << " is out of range.  Must be >= 1.");
    return 0;
  }

  output->ShallowCopy(this->CreateGrid(this->GridNbTicks, this->Scale, this->Origin, this->Normal));
  return 1;
}

//-----------------------------------------------------------------------------
void vtkGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GridSize: " << this->GridNbTicks << std::endl;
  os << indent << "Scale: " << this->Scale << std::endl;
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1] << " " << this->Origin[2]
     << std::endl;
  os << indent << "Normal: " << this->Normal[0] << " " << this->Normal[1] << " " << this->Normal[2]
     << std::endl;
}
