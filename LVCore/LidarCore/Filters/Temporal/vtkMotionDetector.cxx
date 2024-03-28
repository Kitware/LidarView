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
#include <list>
#include <numeric>

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
  /**
   * @brief The GaussianMixture class constructs a gaussian mixture mode
   * to evaluate the probability that a new point is belong to background
   * Each gaussian distribution represents a cluster with a weight
   * The background is the cluster which has a large value of weight / sigma
   */
  class GaussianMixture
  {
  public:
    // Default constructor
    GaussianMixture() = default;

    void Reset() { this->Gaussians.clear(); }

    void SetMaxTTL(int windowSize)
    {
      if (this->MaxTTL != windowSize)
      {
        this->MaxTTL = windowSize;
        for (auto& gaussian : this->Gaussians)
          gaussian.MaxTTL = this->MaxTTL;
      }
    }

    /**
     * @brief Evaluate the closest cluster of a point and store the iterator of the cluster
     * Estimate if the point is a motion point by checking the motion label of its closest cluster
     * Estimate the probability that the point is a motion point
     * @return The motion proba of the point
     * probas: probabilities that the point belongs to each cluster
     * motionEstim: estimated motion label of the point
     */
    double Evaluate(double x, bool isInitStep, std::vector<double>& probas, bool& motionEstim)
    {
      double motionProba = 0.;
      // No cluster has been added, can not look for the closest cluster
      // All points are considered as background point at the initialization step
      // After initialization step, a new point is considered as motion point by defaut
      if (this->Gaussians.empty())
      {
        motionEstim = isInitStep ? false : true;
        motionProba = motionEstim ? 1. : 0.;
        return motionProba;
      }

      // If the cluster is not empty, find the closest cluster for the point
      // Store the iterator in ItMaxProba
      probas.clear();
      double maxProba = 0.;
      this->ItMaxProba = this->Gaussians.begin();
      for (std::list<Gaussian>::iterator it = this->Gaussians.begin(); it != this->Gaussians.end();
           ++it)
      {
        probas.emplace_back(it->Weight * (*it)(x));
        if (probas.back() > maxProba)
        {
          maxProba = probas.back();
          this->ItMaxProba = it;
        }
      }
      double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);

      // During initialization, all points are considered as background
      // No need to check the motion label of the closest cluster
      if (isInitStep)
      {
        motionEstim = false;
        return 0.;
      }

      // After initialization step
      // If the new point is far from existing clusters, it is considered as motion point
      if (std::fabs(x - this->ItMaxProba->Mean) > 3 * this->ItMaxProba->Sigma)
      {
        motionEstim = true;
        return 1.;
      }

      // The closest cluster is a background cluster, the point is labeled as background
      if (!this->ItMaxProba->IsMotion)
      {
        motionEstim = false;
        motionProba = sumProba < 1e-6 ? 0. : (1 - maxProba / sumProba);
        return motionProba;
      }

      // The closest cluster is a motion cluster
      // Check if or not the closest cluster becomes a background cluster
      // Suppose background points appear more often than motion points, so
      // the gaussian which represents a background cluster should have a large
      // weight and small sigma. The weight / sigma value is used to evaluate background
      motionProba = sumProba < 1e-6 ? 0. : maxProba / sumProba;
      double evalBackground = this->ItMaxProba->Weight / this->ItMaxProba->Sigma;
      if (evalBackground > 5.)
      {
        this->ItMaxProba->IsMotion = false;
        motionProba = 1 - motionProba;
        return motionProba;
      }
      for (std::list<Gaussian>::iterator it = this->Gaussians.begin(); it != this->Gaussians.end();
           ++it)
      {
        if (it != this->ItMaxProba && !it->IsMotion && evalBackground > (it->Weight / it->Sigma))
        {
          this->ItMaxProba->IsMotion = false;
          motionProba = 1 - motionProba;
        }
      }
      motionEstim = this->ItMaxProba->IsMotion;
      return motionProba;
    }
    /**
     * @brief Add the new point into gaussian mixture model
     * If the model is empty, add a new cluster with weight = 1
     * If the point is too far from existing clusters, add a new cluster with appropriate weight
     * Otherwise, add the point into model and update parameters
     */
    void AddPoint(double x, bool motionEstim, std::vector<double>& probas)
    {
      // Create a new gaussian if the gaussian mixture model is empty
      if (this->Gaussians.empty())
      {
        Gaussian newGaussian(x, 0.2, this->MaxTTL, 1, motionEstim);
        this->Gaussians.push_back(newGaussian);
        this->ItMaxProba = this->Gaussians.begin();
        return;
      }

      // Update current gaussian mixture model if the new point is close to a gaussian cluster,
      if (std::fabs(x - this->ItMaxProba->Mean) < (3. * this->ItMaxProba->Sigma))
      {
        double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);
        int idProba = 0;
        for (auto& gaussian : this->Gaussians)
        {
          gaussian.UpdateParams(x, probas[idProba] / sumProba);
          ++idProba;
        }
        this->ResetTTL();
        return;
      }

      // Create a new gaussian if the new point is far from existing distributions
      Gaussian newGaussian(x, 0.2, this->MaxTTL, this->ItMaxProba->N + 1, motionEstim);
      probas.clear();
      for (auto gaussian : this->Gaussians)
      {
        probas.emplace_back(gaussian(x));
      }
      probas.emplace_back(newGaussian(x));
      double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);
      int idProba = 0;
      // Update existing gaussians with new weights
      for (std::list<Gaussian>::iterator it = this->Gaussians.begin(); it != this->Gaussians.end();
           ++it)
      {
        it->UpdateParams(x, probas[idProba] / sumProba);
        ++idProba;
      }
      // Set weight for new gaussian and add it to the model
      double newGaussianWeight =
        (probas.back() / sumProba) / static_cast<double>(this->ItMaxProba->N + 1);
      newGaussian.Weight = newGaussianWeight;
      this->Gaussians.push_back(newGaussian);
      this->ItMaxProba = std::prev(this->Gaussians.end());
    }

    /**
     * @brief Update the time to live of the each gaussian in the model
     * If a gaussian is dead, erase it. Then the weight of the remains
     * gaussians need to be normalized and the motion labels need to be updated
     */
    void UpdateTTL()
    {
      if (this->Gaussians.empty())
        return;
      std::list<Gaussian>::iterator it = this->Gaussians.begin();
      double sumWeights = 0;
      while (it != this->Gaussians.end())
      {
        // Check if or not the gaussian is still alive
        bool stillAlive = it->UpdateTTL();

        // Erase the gaussian if it is not alive
        if (!stillAlive)
        {
          // erase current iterator and get next on
          it = this->Gaussians.erase(it);
          // no need to increment iterator here
        }
        else
        {
          sumWeights += it->Weight;
          it++; // increment iterator here
        }
      }
      // Normalise weights and update motion label
      for (auto& gaussian : this->Gaussians)
      {
        gaussian.Weight /= sumWeights;
        if (gaussian.Weight / gaussian.Sigma > 10.)
          gaussian.IsMotion = false;
      }
    }

    /**
     * @brief Reset time to live value to its maximum when a new
     * point is added to a gaussian distribution
     */
    void ResetTTL() { this->ItMaxProba->TTL = this->MaxTTL; }

  private:
    // Maximum number of frames for TTL
    int MaxTTL = 50;

    // Iterator to the cluster which the point has a max proba
    std::list<Gaussian>::iterator ItMaxProba;

    // Gaussian distributions
    std::list<Gaussian> Gaussians;
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