/*=========================================================================

  Program:   LidarView
  Module:    vtkVoxelGridFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// local includes
#include "vtkVoxelGridFilter.h"
#include "vtkVoxelGridProcessor.h"

// vtk includes
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPolyData.h>

// Implementation of the New function
vtkStandardNewMacro(vtkVoxelGridFilter);

//-----------------------------------------------------------------------------
void vtkVoxelGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LeafSize: " << this->LeafSize << endl;
  os << indent << "SamplingMode: " << this->SamplingMode << endl;
}

//-----------------------------------------------------------------------------
int vtkVoxelGridFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  vtkNew<vtkVoxelGridProcessor> voxelGrid;
  voxelGrid->SetLeafSize(this->LeafSize);
  voxelGrid->SetSampling(this->SamplingMode);
  double bounds[6];
  input->GetBounds(bounds);
  voxelGrid->SetBounds(bounds);

  for (int i = 0; i < input->GetPoints()->GetNumberOfPoints(); ++i)
  {
    voxelGrid->AddPoint(input, i);
  }

  output->ShallowCopy(voxelGrid->GetOutput());
  return 1;
}
