// Copyright 2017 Actemium.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMotionDetector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMotionDetector.h"

// VTK
#include <vtkAppendFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolyLine.h>
#include <vtkQuaternion.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkXMLPolyDataReader.h>

// EIGEN
#include <Eigen/Dense>

// STD
#include <iostream>

constexpr unsigned int LIDAR_FRAME_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int MOTION_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int CLUSTERS_OUTPUT_PORT = 1;
constexpr unsigned int OUTPUT_PORT_COUNT = 2;

// Implementation of the New function
vtkStandardNewMacro(vtkMotionDetector)

//----------------------------------------------------------------------------
vtkMotionDetector::vtkMotionDetector()
  : Internals(new vtkMotionDetector::vtkInternals())
{
  // One input port
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);

  // The accumulation of stabilized frames
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//----------------------------------------------------------------------------
vtkMotionDetector::~vtkMotionDetector() = default;

//-----------------------------------------------------------------------------
int vtkMotionDetector::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkMotionDetector::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == MOTION_POINTS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == CLUSTERS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkMotionDetector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkMotionDetector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get input data
  vtkPolyData* input =
    vtkPolyData::GetData(inputVector[LIDAR_FRAME_INPUT_PORT]->GetInformationObject(0));
  // Check if input is a multiblock
  if (!input)
  {
    vtkMultiBlockDataSet* mb =
      vtkMultiBlockDataSet::GetData(inputVector[LIDAR_FRAME_INPUT_PORT], 0);
    // Extract first block if it is a vtkPolyData
    input = vtkPolyData::SafeDownCast(mb->GetBlock(0));
  }
  // If the input could not be cast, return
  if (!input)
  {
    vtkErrorMacro(<< "Unable to cast input into a vtkPolyData");
    return 0;
  }

  // Get the output
  vtkPolyData* output = vtkPolyData::GetData(outputVector->GetInformationObject(0));
  output->ShallowCopy(input);

  // Add the new points into the Gaussian Map
  this->AddFrame(output);

  return 1;
}