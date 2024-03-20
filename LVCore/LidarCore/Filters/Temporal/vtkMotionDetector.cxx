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

class vtkMotionDetector::vtkInternals
{
public:
  /**
   * @brief The gaussian struct is used to compute the normal distribution:
   * 1.0 / (sigma * sqrt(2 * pi)) * exp((x - mean) ^ 2) / (2.0 * sigma^2))
   * When a new point is added, the sigma and the mean will be updated
   * The weight parameter is to store the weight of each gaussian in GMM
   */
  struct Gaussian
  {
    // Constructor
    Gaussian() = default;
    Gaussian(double mean,
      double sigma,
      int maxTTL,
      unsigned int nb = 1,
      bool isMotion = false,
      double weight = 1.0)
      : Mean(mean)
      , Sigma(sigma)
      , MaxTTL(maxTTL)
      , TTL(maxTTL)
      , N(nb)
      , IsMotion(isMotion)
      , Weight(weight){};

    // Mean of the gaussian
    double Mean = 0.;

    // Standard deviation of the gaussian
    double Sigma = 0.2;

    // Maximum number of frames for TTL
    int MaxTTL = 50;

    // Time to live of the gaussian
    int TTL = 50;

    // Number of samples added into gaussian
    unsigned int N = 0;

    // If or not the gaussian cluster represent a motion area
    bool IsMotion = false;

    // Weight of the distribution
    double Weight = 1.0;

    // Get gaussian value of point x
    double operator()(double x)
    {
      return 1.0 / (this->Sigma * sqrt(2 * vtkMath::Pi())) *
        exp(-std::pow(x - this->Mean, 2) / (2.0 * std::pow(this->Sigma, 2)));
    }

    // Update parameters using new value and the weight of this value
    void UpdateParams(double x, double weightX)
    {
      // Update the weight
      double oldWeight = this->Weight;
      double sumWeight = static_cast<double>(this->N) * oldWeight;
      this->Weight = (sumWeight + weightX) / (static_cast<double>(this->N + 1));

      // Update the mean
      double oldMean = this->Mean;
      this->Mean = oldMean + weightX * (x - oldMean) / (sumWeight + weightX);

      // Update the standard deviation using Welford's method
      this->Sigma = std::sqrt(
        (sumWeight * std::pow(this->Sigma, 2) + weightX * (x - oldMean) * (x - this->Mean)) /
        (sumWeight + weightX));

      // Update the number of sample
      this->N += 1;
    }

    // Update the time to live
    // Return false if TTL falls to zero value
    bool UpdateTTL()
    {
      this->TTL -= 1;
      return (this->TTL >= 0);
    }
  };
};

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