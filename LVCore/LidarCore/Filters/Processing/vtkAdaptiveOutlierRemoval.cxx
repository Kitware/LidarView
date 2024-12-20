/*=========================================================================

  Program: LidarView
  Module:  vtkAdaptiveOutlierRemoval.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// LOCAL
#include "vtkAdaptiveOutlierRemoval.h"
#include "KDTreeVTKAdaptor.h"

// VTK
#include <vtkInformation.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>

constexpr unsigned int POINTS_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int FILTERED_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int OUTPUT_PORT_COUNT = 1;

vtkStandardNewMacro(vtkAdaptiveOutlierRemoval)

//-----------------------------------------------------------------------------
vtkAdaptiveOutlierRemoval::vtkAdaptiveOutlierRemoval()
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//-----------------------------------------------------------------------------
int vtkAdaptiveOutlierRemoval::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAdaptiveOutlierRemoval::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get IO
  vtkPolyData* inputPointcloud = vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT], 0);
  vtkPolyData* outputPointcloud = vtkPolyData::GetData(outputVector, FILTERED_POINTS_OUTPUT_PORT);

  return 1;
}
