/*=========================================================================

  Program:   LidarView
  Module:    vtkCurbDetectorOffline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCurbDetectorOffline.h"

#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

vtkStandardNewMacro(vtkCurbDetectorOffline);

//-----------------------------------------------------------------------------
vtkCurbDetectorOffline::vtkCurbDetectorOffline()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
int vtkCurbDetectorOffline::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCurbDetectorOffline::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  return 1;
}
