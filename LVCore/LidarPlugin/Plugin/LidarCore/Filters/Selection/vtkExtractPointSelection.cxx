/*=========================================================================

  Program:   LidarView
  Module:    vtkExtractPointSelection.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// local includes
#include "vtkExtractPointSelection.h"

// vtk includes
#include <vtkConvertToPointCloud.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPVSingleOutputExtractSelection.h>
#include <vtkPolyData.h>
#include <vtkSelection.h>

// Implementation of the New function
vtkStandardNewMacro(vtkExtractPointSelection);

//----------------------------------------------------------------------------
vtkExtractPointSelection::vtkExtractPointSelection()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
int vtkExtractPointSelection::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkExtractPointSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkExtractPointSelection::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Extract selection
  vtkNew<vtkPVSingleOutputExtractSelection> extractSelection;
  extractSelection->SetInputData(0, vtkDataSet::GetData(inputVector[0]->GetInformationObject(0)));
  extractSelection->SetInputData(1, vtkSelection::GetData(inputVector[1]->GetInformationObject(0)));
  extractSelection->Update();

  vtkDataSet* input = vtkDataSet::SafeDownCast(extractSelection->GetOutput(0));

  // Convert to polydata
  vtkNew<vtkConvertToPointCloud> convertToPointCloud;
  convertToPointCloud->SetInputData(input);
  // Generate one vertex cell by point to be coherent with LidarView's pointcloud
  convertToPointCloud->SetCellGenerationMode(vtkConvertToPointCloud::VERTEX_CELLS);
  convertToPointCloud->Update();

  vtkPolyData* output = vtkPolyData::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(convertToPointCloud->GetOutput());

  return 1;
}
