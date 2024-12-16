/*=========================================================================

  Program: LidarView
  Module:  vtkClusteringAndTracking.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClusteringAndTracking.h"
#include "vtkPointsPCA.h"

// VTK
#include <vtkAppendFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCenterOfMass.h>
#include <vtkCleanPolyData.h>
#include <vtkCubeSource.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkEuclideanClusterExtraction.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPCAStatistics.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>
#include <vtkQuaternion.h>
#include <vtkRadiusOutlierRemoval.h>
#include <vtkRemovePolyData.h>
#include <vtkStringArray.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>

// STD
#include <iomanip>
#include <iostream>
#include <list>
#include <numeric>

constexpr unsigned int POINTCLOUD_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int CLUSTERS_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int CLUSTERS_OUTPUT_PORT = 1;
constexpr unsigned int CLUSTERS_TEXT_OUTPUT_PORT = 2;
constexpr unsigned int OUTPUT_PORT_COUNT = 3;

// Implementation of the New function
vtkStandardNewMacro(vtkClusteringAndTracking)

//----------------------------------------------------------------------------
vtkClusteringAndTracking::vtkClusteringAndTracking()
{
  // One input port
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);

  // The accumulation of stabilized frames
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//----------------------------------------------------------------------------
vtkClusteringAndTracking::~vtkClusteringAndTracking() = default;

//-----------------------------------------------------------------------------
int vtkClusteringAndTracking::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkClusteringAndTracking::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == CLUSTERS_POINTS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == CLUSTERS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
    return 1;
  }
  if (port == CLUSTERS_TEXT_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkClusteringAndTracking::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get input data
  vtkPolyData* input =
    vtkPolyData::GetData(inputVector[POINTCLOUD_INPUT_PORT]->GetInformationObject(0));
  // Check if input is a multiblock
  if (!input)
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::GetData(inputVector[POINTCLOUD_INPUT_PORT], 0);
    // Extract first block if it is a vtkPolyData
    input = vtkPolyData::SafeDownCast(mb->GetBlock(0));
  }
  // If the input could not be cast, return
  if (!input || input->GetNumberOfPoints() < 1)
  {
    vtkErrorMacro(<< "Unable to cast input into a vtkPolyData");
    return 0;
  }

  // Get the output
  vtkPolyData* clustersPointsOutput =
    vtkPolyData::GetData(outputVector, CLUSTERS_POINTS_OUTPUT_PORT);
  vtkMultiBlockDataSet* clustersOutput =
    vtkMultiBlockDataSet::GetData(outputVector, CLUSTERS_OUTPUT_PORT);
  vtkTable* clusterInfoOutput = vtkTable::GetData(outputVector, CLUSTERS_TEXT_OUTPUT_PORT);

  return 1;
}
