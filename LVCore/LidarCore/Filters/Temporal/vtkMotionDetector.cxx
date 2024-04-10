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
#include <vtkCubeSource.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkEuclideanClusterExtraction.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolyLine.h>
#include <vtkQuaternion.h>
#include <vtkRadiusOutlierRemoval.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkStringArray.h>
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

namespace
{
constexpr const char* HESAI_NAME = "Hesai";
constexpr const char* LIVOX_NAME = "Livox";
constexpr const char* VELODYNE_NAME = "Velodyne";
constexpr const char* UNKNOWN_NAME = "Unknown";
}

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
      // Store the iterator in ItClosest
      probas.clear();
      double closestProba = 0.;
      double minDistance = FLT_MAX;
      this->ItClosest = this->Gaussians.begin();
      for (std::list<Gaussian>::iterator it = this->Gaussians.begin(); it != this->Gaussians.end();
           ++it)
      {
        probas.emplace_back(it->Weight * (*it)(x));
        double dist = std::fabs(x - it->Mean);
        if (dist < minDistance)
        {
          minDistance = dist;
          closestProba = probas.back();
          this->ItClosest = it;
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
      if (std::fabs(x - this->ItClosest->Mean) > 3 * this->ItClosest->Sigma)
      {
        motionEstim = true;
        return 1.;
      }

      // The closest cluster is a background cluster, the point is labeled as background
      if (!this->ItClosest->IsMotion)
      {
        motionEstim = false;
        motionProba = sumProba < 1e-6 ? 0. : (1 - closestProba / sumProba);
        return motionProba;
      }

      // The closest cluster is a motion cluster
      // Check if or not the closest cluster becomes a background cluster
      // Suppose background points appear more often than motion points, so
      // the gaussian which represents a background cluster should have a large
      // weight and small sigma. The weight / sigma value is used to evaluate background
      motionProba = sumProba < 1e-6 ? 0. : closestProba / sumProba;
      double evalBackground = this->ItClosest->Weight / this->ItClosest->Sigma;
      if (evalBackground > 5.)
      {
        this->ItClosest->IsMotion = false;
        motionProba = 1 - motionProba;
        return motionProba;
      }
      for (std::list<Gaussian>::iterator it = this->Gaussians.begin(); it != this->Gaussians.end();
           ++it)
      {
        if (it != this->ItClosest && !it->IsMotion && evalBackground > (it->Weight / it->Sigma))
        {
          this->ItClosest->IsMotion = false;
          motionProba = 1 - motionProba;
        }
      }
      motionEstim = this->ItClosest->IsMotion;
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
        this->ItClosest = this->Gaussians.begin();
        return;
      }

      // Update current gaussian mixture model if the new point is close to a gaussian cluster,
      if (std::fabs(x - this->ItClosest->Mean) < (3. * this->ItClosest->Sigma))
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
      Gaussian newGaussian(x, 0.2, this->MaxTTL, this->ItClosest->N + 1, motionEstim);
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
        (probas.back() / sumProba) / static_cast<double>(this->ItClosest->N + 1);
      newGaussian.Weight = newGaussianWeight;
      this->Gaussians.push_back(newGaussian);
      this->ItClosest = std::prev(this->Gaussians.end());
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
    void ResetTTL() { this->ItClosest->TTL = this->MaxTTL; }

  private:
    // Maximum number of frames for TTL
    int MaxTTL = 50;

    // Iterator to the cluster which the mean value is the closest to the point
    std::list<Gaussian>::iterator ItClosest;

    // Gaussian distributions
    std::list<Gaussian> Gaussians;
  };

  // The spherical depth map with gaussian mixture model
  std::vector<GaussianMixture> Map;

  /**
   * Spherical map bounds in degrees. They depend on lidar model
   * VLP-16 :      Vertical ~[-15, 15]  Azimuth ~[0, 360]
   * VLP-32c:      Vertical ~[-25, 15]  Azimuth ~[0, 360]
   * Livox HAP   : Vertical ~[70, 105] Azimuth ~[-60, 60]
   * Livox MID360: Vertical ~[30, 100] Azimuth ~[-180, 180]
   * */
  double VerticalBounds[2] = { -90., 90. };
  double AzimuthBounds[2] = { 0., 360. };

  /*
   * Resolution in degrees. Depend on lidar resolution
   */
  double VerticalResolution = 1.25;
  double AzimuthResolution = 0.1;

  /**
   * Number of sample points along vertical angle and azimuth angle
   * They are computed by Bounds and Resolution parameters
   */
  unsigned int NbAzimuth;
  unsigned int NbVertical;

  /**
   * Window size to define time to live of a gaussian distribution
   */
  int WindowSize = 50;

  /**
   * Enum of lidar vendor
   */
  enum LidarVendor
  {
    UNKNOWN = 0,
    LIVOX = 1,
    VELODYNE = 2,
    HESAI = 3,
  };
  const std::map<std::string, LidarVendor> LidarVendors = { { LIVOX_NAME, LidarVendor::LIVOX },
    { VELODYNE_NAME, LidarVendor::VELODYNE },
    { HESAI_NAME, LidarVendor::HESAI },
    { UNKNOWN_NAME, LidarVendor::UNKNOWN } };
  LidarVendor Lidar = LidarVendor::UNKNOWN;

  /**
   * Compute bounds from first frame of lidar data
   * Azimuth bounds and Vertical bounds
   */
  void ComputeBounds(vtkPolyData* polydata)
  {
    // Get bounds of azimuth angle and vertical angle
    switch (this->Lidar)
    {
      case LidarVendor::VELODYNE:
      {
        polydata->GetPointData()->GetArray("azimuth")->GetRange(this->AzimuthBounds);
        polydata->GetPointData()->GetArray("vertical_angle")->GetRange(this->VerticalBounds);
        this->AzimuthBounds[0] = std::floor(this->AzimuthBounds[0] / 100.);
        this->AzimuthBounds[1] = std::ceil(this->AzimuthBounds[1] / 100.);
        this->VerticalBounds[0] = std::floor(this->VerticalBounds[0]);
        this->VerticalBounds[1] = std::ceil(this->VerticalBounds[1]);
        break;
      }
      case LidarVendor::LIVOX:
      case LidarVendor::HESAI:
      {
        double point[3];
        this->AzimuthBounds[0] = DBL_MAX;
        this->AzimuthBounds[1] = -DBL_MAX;
        this->VerticalBounds[0] = DBL_MAX;
        this->VerticalBounds[1] = -DBL_MAX;
        for (auto id = 0; id < polydata->GetNumberOfPoints(); ++id)
        {
          polydata->GetPoint(id, point);
          double r = polydata->GetPointData()->GetArray("distance_m")->GetTuple1(id);
          double azimuth = vtkMath::DegreesFromRadians(std::atan2(point[1], point[0]));
          double vertical = vtkMath::DegreesFromRadians(std::acos(point[2] / r));
          this->AzimuthBounds[0] = std::min(azimuth, this->AzimuthBounds[0]);
          this->AzimuthBounds[1] = std::max(azimuth, this->AzimuthBounds[1]);
          this->VerticalBounds[0] = std::min(vertical, this->VerticalBounds[0]);
          this->VerticalBounds[1] = std::max(vertical, this->VerticalBounds[1]);
        }
        this->AzimuthBounds[0] = std::floor(this->AzimuthBounds[0]);
        this->AzimuthBounds[1] = std::ceil(this->AzimuthBounds[1]);
        this->VerticalBounds[0] = std::floor(this->VerticalBounds[0]);
        this->VerticalBounds[1] = std::ceil(this->VerticalBounds[1]);
        break;
      }
      default:
      {
        this->AzimuthBounds[0] = 0;
        this->AzimuthBounds[1] = 360;
        this->VerticalBounds[0] = -90;
        this->VerticalBounds[1] = 90;
      }
    }
    this->ResetMap();
    vtkLog(INFO,
      "The azimuth  angle range is [ "
        << this->AzimuthBounds[0] << ", " << this->AzimuthBounds[1] << "]\n"
        << "The vertical angle range is [ " << this->VerticalBounds[0] << ", "
        << this->VerticalBounds[1] << "]\n");
  }

  // Init the Spehrical map
  void InitMap()
  {
    // Phi Bounds
    this->VerticalBounds[0] = -90;
    this->VerticalBounds[1] = 90;

    // Theta Bounds
    this->AzimuthBounds[0] = -180;
    this->AzimuthBounds[1] = 180;

    // Default VerticalResolution / AzimuthResolution values
    this->VerticalResolution = 1.0;
    this->AzimuthResolution = 0.1;

    // reset internal parameters
    this->ResetMap();
  }

  /**
   *Reset the Spehrical map using the current parameters
   */
  void ResetMap()
  {
    // Update quantum
    this->NbVertical = static_cast<unsigned int>(
      (this->VerticalBounds[1] - this->VerticalBounds[0]) / this->VerticalResolution);
    this->NbAzimuth = static_cast<unsigned int>(
      (this->AzimuthBounds[1] - this->AzimuthBounds[0]) / this->AzimuthResolution);

    // Reset the map
    for (auto& mixture : this->Map)
    {
      mixture.Reset();
    }
    this->Map.clear();
    this->Map.resize(this->NbVertical * this->NbAzimuth);
    for (auto& mixture : this->Map)
    {
      mixture.SetMaxTTL(this->WindowSize);
    }
  }

  /*
   * Convert cartesian coordinates of point x into its spherical coordinates
   */
  Eigen::Matrix<double, 3, 1> GetSphericalCoordinates(const Eigen::Matrix<double, 3, 1>& x)
  {
    // Base of R3 used. In some case it can be changed
    Eigen::Matrix<double, 3, 1> ez{ 0, 0, 1 };
    Eigen::Matrix<double, 3, 1> ey{ 0, 1, 0 };
    Eigen::Matrix<double, 3, 1> ex{ 1, 0, 0 };

    // Center of the coordinate system using the ex, ey, ez base
    Eigen::Matrix<double, 3, 1> origin{ 0, 0, 0 };

    // Express the current point in the local reference frame
    // designed by the internal base and origin
    Eigen::Matrix<double, 3, 3> transR;
    transR << ex(0), ey(0), ez(0), ex(1), ey(1), ez(1), (2), ey(2), ez(2);
    Eigen::Matrix<double, 3, 1> xLocal = transR.transpose() * (x - origin);

    // Compute the vector length
    double r = xLocal.norm();

    // Normalize the vector if it is not null
    if (r > 1e-4)
    {
      xLocal.normalize();
    }

    // Project CX onto the (ex, ey) plane
    Eigen::Matrix<double, 3, 1> projX = xLocal.dot(ex) * ex + xLocal.dot(ey) * ey;
    projX.normalize();

    // Compute Phi angle (vertical direction)
    double cosPhi = xLocal.dot(ez);
    double sinPhi = xLocal.cross(ez).transpose() * xLocal.cross(ez).normalized();
    double phi = vtkMath::DegreesFromRadians(std::atan2(sinPhi, cosPhi));

    // Compute theta angle (azimuth direction)
    double cosTheta = projX.dot(ex);
    double sinTheta = projX.cross(ex).dot(ey.cross(ex).normalized());
    double theta = vtkMath::DegreesFromRadians(std::atan2(sinTheta, cosTheta));

    Eigen::Matrix<double, 3, 1> sphericalCoords;
    sphericalCoords << r, theta, phi;
    return sphericalCoords;
  }

  /*
   * Update the TTL of the gaussian mixture model
   */
  void UpdateTTL()
  {
    for (unsigned int k = 0; k < this->Map.size(); ++k)
    {
      this->Map[k].UpdateTTL();
    }
  }
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

  // Initialize the internal parameters
  this->Internals->InitMap();
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
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
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
void vtkMotionDetector::Reset()
{
  this->Internals->ResetMap();
  this->NbProcessedFrames = 0;
}

//----------------------------------------------------------------------------
void vtkMotionDetector::SetVerticalResolution(double verticalReso)
{
  if (this->Internals->VerticalResolution != verticalReso)
  {
    this->Internals->VerticalResolution = verticalReso;
    this->Internals->ResetMap();
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::SetAzimuthResolution(double azimuthReso)
{
  if (this->Internals->AzimuthResolution != azimuthReso)
  {
    this->Internals->AzimuthResolution = azimuthReso;
    this->Internals->ResetMap();
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::SetWindowSize(int windowSize)
{
  if (this->Internals->WindowSize != windowSize)
  {
    this->Internals->WindowSize = windowSize;
    for (auto& gaussians : this->Internals->Map)
    {
      gaussians.SetMaxTTL(this->Internals->WindowSize);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::EstimateMotion(vtkSmartPointer<vtkPolyData> polydata)
{
  vtkSmartPointer<vtkDoubleArray> motionLabel = vtkSmartPointer<vtkDoubleArray>::New();
  motionLabel->SetName("Motion_label");

  double point[3];
  Eigen::Matrix<double, 3, 1> sphericalPoint;
  this->NbMotionPoints = 0;
  for (auto id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    // Get point and compute its spherical coordinates
    polydata->GetPoint(id, point);
    switch (this->Internals->Lidar)
    {
      case vtkInternals::LidarVendor::VELODYNE:
      {
        double r = polydata->GetPointData()->GetArray("distance_m")->GetTuple1(id);
        double theta = polydata->GetPointData()->GetArray("azimuth")->GetTuple1(id);
        double phi = polydata->GetPointData()->GetArray("vertical_angle")->GetTuple1(id);
        theta = theta / 100.;
        sphericalPoint << r, theta, phi;
        break;
      }
      case vtkInternals::LidarVendor::LIVOX:
      case vtkInternals::LidarVendor::HESAI:
      {
        double r = polydata->GetPointData()->GetArray("distance_m")->GetTuple1(id);
        double theta = vtkMath::DegreesFromRadians(std::atan2(point[1], point[0]));
        double phi = vtkMath::DegreesFromRadians(std::acos(point[2] / r));
        sphericalPoint << r, theta, phi;
        break;
      }
      default:
      {
        Eigen::Matrix<double, 3, 1> cartesianPoint{ point[0], point[1], point[2] };
        sphericalPoint = this->Internals->GetSphericalCoordinates(cartesianPoint);
        break;
      }
    }

    int isMotion = 0;
    // Check if or not the point is in the detection range
    if (sphericalPoint(0) <= this->DetectionRange)
    {
      // Convert spherical coordinates to spherical map coordinates
      unsigned int idxVertical =
        static_cast<int>(std::floor((sphericalPoint(2) - this->Internals->VerticalBounds[0]) /
          this->Internals->VerticalResolution));
      unsigned int idxAzimuth =
        static_cast<int>(std::floor((sphericalPoint(1) - this->Internals->AzimuthBounds[0]) /
          this->Internals->AzimuthResolution));
      idxVertical = std::min(idxVertical, this->Internals->NbVertical - 1);
      idxAzimuth = std::min(idxAzimuth, this->Internals->NbAzimuth - 1);

      bool isInitStep = this->NbProcessedFrames < this->InitializationTime ? true : false;
      // Evaluate the mixture model on the current data
      // It returns the motion probability of the point
      // and stores the iterator of the cluster which give the max proba
      std::vector<double> probas;
      bool motionEstim = false;
      this->Internals->Map[idxAzimuth + this->Internals->NbAzimuth * idxVertical].Evaluate(
        sphericalPoint(0), isInitStep, probas, motionEstim);
      isMotion = motionEstim ? 1 : 0;
      this->NbMotionPoints += isMotion;

      // Add the depth to the correct "pixel" and update parameters of the model
      this->Internals->Map[idxAzimuth + this->Internals->NbAzimuth * idxVertical].AddPoint(
        sphericalPoint(0), motionEstim, probas);
    }

    motionLabel->InsertNextTuple1(isMotion);
  }

  // Update time to live of the gaussians
  this->Internals->UpdateTTL();

  // Add motion label field to pointcloud
  polydata->GetPointData()->AddArray(motionLabel);
}

//-----------------------------------------------------------------------------
void vtkMotionDetector::ExtractClusters(vtkSmartPointer<vtkPolyData> input,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput)
{
  // Get motion points
  if (this->NbMotionPoints == 0)
  {
    return;
  }
  vtkNew<vtkPoints> motionPoints;
  motionPoints->SetNumberOfPoints(this->NbMotionPoints);
  vtkNew<vtkPolyData> motionPolyData;
  motionPolyData->SetPoints(motionPoints);
  vtkIdType pointIndex = 0;
  for (vtkIdType k = 0; k < input->GetNumberOfPoints(); ++k)
  {
    if (input->GetPointData()->GetArray("Motion_label")->GetTuple1(k))
    {
      double point[3];
      input->GetPoint(k, point);
      motionPoints->SetPoint(pointIndex, point);
      ++pointIndex;
    }
  }

  // Copy frame infomation from input
  for (vtkIdType idxArray = 0; idxArray < input->GetPointData()->GetNumberOfArrays(); ++idxArray)
  {
    char* fieldName = input->GetPointData()->GetArray(idxArray)->GetName();
    auto arrayTmp = input->GetPointData()->GetArray(idxArray)->NewInstance();
    arrayTmp->Resize(motionPolyData->GetNumberOfPoints());
    arrayTmp->SetName(fieldName);
    for (vtkIdType idx = 0; idx < input->GetNumberOfPoints(); ++idx)
    {
      if (input->GetPointData()->GetArray("Motion_label")->GetTuple1(idx) == 0)
        continue;
      double* value = input->GetPointData()->GetArray(idxArray)->GetTuple(idx);
      arrayTmp->InsertNextTuple(value);
    }
    motionPolyData->GetPointData()->AddArray(arrayTmp);
  }

  // Remove outlier
  vtkNew<vtkRadiusOutlierRemoval> removal;
  removal->SetInputData(motionPolyData);
  removal->SetRadius(this->RemovalOutlierRadius);
  removal->SetNumberOfNeighbors(this->RemovalOutlierNeighbors);
  removal->Update();
  if (removal->GetOutput()->GetNumberOfPoints() == 0)
    return;

  // Extract cluster
  vtkNew<vtkEuclideanClusterExtraction> cluster;
  cluster->SetInputConnection(removal->GetOutputPort());
  cluster->SetExtractionModeToAllClusters();
  cluster->SetRadius(this->ClusterRadius);
  cluster->ColorClustersOn();
  cluster->Update();

  // Create output with vertices
  vtkNew<vtkPolyData> output;
  output->ShallowCopy(cluster->GetOutput());
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(output->GetNumberOfPoints());
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(1, connectivity);
  output->SetVerts(cellArray);
  for (vtkIdType k = 0; k < output->GetNumberOfPoints(); ++k)
  {
    connectivity->SetValue(k, k);
  }

  // Compute cluster stats: size, mean depth, mean intensity etc
  this->Clusters.clear();
  int numClusters = cluster->GetNumberOfExtractedClusters();
  for (int clusterId = 0; clusterId < numClusters; ++clusterId)
  {
    ClusterStats clusterInfo;
    clusterInfo.ClusterId = clusterId;
    // Calculate the average depth value and bounding box for this cluster
    double depth = 0.0;
    double intensity = 0.0;
    int nbClusterPoints = 0;
    for (vtkIdType pointId = 0; pointId < output->GetNumberOfPoints(); ++pointId)
    {
      auto currClusterId = output->GetPointData()->GetArray("ClusterId")->GetTuple1(pointId);
      if (currClusterId != clusterId)
        continue;
      // Accumulate depth value and intensity value
      // todo: add sanity check
      depth += output->GetPointData()->GetArray("distance_m")->GetTuple1(pointId);
      intensity += output->GetPointData()->GetArray("intensity")->GetTuple1(pointId);
      ++nbClusterPoints;
      // Compute boundingbox {xmin, xmax, ymin, ymax, zmin, zmax}
      double point[3];
      output->GetPoint(pointId, point);
      for (int dim = 0; dim < 3; ++dim)
      {
        // Update min
        if (point[dim] < clusterInfo.BoundingBox[2 * dim])
          clusterInfo.BoundingBox[2 * dim] = point[dim];
        // Updat max
        if (point[dim] > clusterInfo.BoundingBox[2 * dim + 1])
          clusterInfo.BoundingBox[2 * dim + 1] = point[dim];
      }
    }
    depth /= static_cast<double>(nbClusterPoints);
    intensity /= static_cast<double>(nbClusterPoints);
    clusterInfo.MeanDepth = depth;
    clusterInfo.MeanIntensity = intensity;
    for (int dim = 0; dim < 3; ++dim)
    {
      clusterInfo.BoxSize[dim] =
        clusterInfo.BoundingBox[2 * dim + 1] - clusterInfo.BoundingBox[2 * dim];
      clusterInfo.BoxCenter[dim] = clusterInfo.BoxSize[dim] / 2 + clusterInfo.BoundingBox[2 * dim];
    }
    this->Clusters.emplace_back(clusterInfo);
  }
  // Sort clusters based on their average depth values
  std::sort(Clusters.begin(),
    Clusters.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.MeanDepth < cluster2.MeanDepth; });
  // Assign new cluster IDs based on the sorted order
  std::vector<int> newClusterIds(numClusters);
  for (int i = 0; i < numClusters; ++i)
  {
    newClusterIds[this->Clusters[i].ClusterId] = i;
  }
  for (int i = 0; i < numClusters; ++i)
  {
    this->Clusters[i].ClusterId = i;
  }
  // Reset cluster id
  auto clusterIdArray = output->GetPointData()->GetArray("ClusterId");
  for (vtkIdType pointId = 0; pointId < output->GetNumberOfPoints(); ++pointId)
  {
    auto clusterId = clusterIdArray->GetTuple1(pointId);
    output->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, newClusterIds[clusterId]);
  }

  // Label cluster by geometry dimension
  for (auto& cluster : this->Clusters)
  {
    if (cluster.BoxSize[2] >= 0.8 && cluster.BoxSize[2] <= 2.2 &&
      ((cluster.BoxSize[0] >= 0.2 && cluster.BoxSize[0] <= 0.7) ||
        (cluster.BoxSize[1] >= 0.2 && cluster.BoxSize[1] <= 0.7)))
    {
      cluster.ClusterLabel = vtkMotionDetector::Label::HUMAN;
    }
    else
    {
      cluster.ClusterLabel = vtkMotionDetector::Label::OTHERS;
    }
  }

  // Add bounding box for each cluster into output
  int blockId = 0;
  for (const auto& cluster : this->Clusters)
  {
    vtkNew<vtkCubeSource> cubeSource;
    vtkNew<vtkPolyData> source;
    cubeSource->SetBounds(cluster.BoundingBox);
    cubeSource->SetCenter(cluster.BoxCenter);
    cubeSource->Update();
    source->ShallowCopy(cubeSource->GetOutput());
    std::string blockName("Cluster-" + std::to_string(cluster.ClusterId));
    clustersOutput->SetBlock(blockId, source);
    clustersOutput->GetMetaData(blockId)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());
    ++blockId;
  }

  // Print clusters info
}

//-----------------------------------------------------------------------------
int vtkMotionDetector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkLog(INFO, "Processing frame #" << this->NbProcessedFrames << "\n");

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

  if (!this->IdentifyInputArrays(input))
  {
    vtkErrorMacro(<< "Unable to identify LiDAR arrays to use.");
    return 0;
  }

  // Get the output
  vtkPolyData* motionPointsOutput = vtkPolyData::GetData(outputVector, MOTION_POINTS_OUTPUT_PORT);
  vtkMultiBlockDataSet* clustersOutput =
    vtkMultiBlockDataSet::GetData(outputVector, CLUSTERS_OUTPUT_PORT);
  motionPointsOutput->ShallowCopy(input);

  // Compute azimuth and vertical angles bounds
  if (this->NbProcessedFrames == 0)
    this->Internals->ComputeBounds(input);

  // Estimate probability of a point and update GMM
  this->EstimateMotion(motionPointsOutput);

  // Extract clusters on the motion points
  this->ExtractClusters(motionPointsOutput, clustersOutput);

  ++this->NbProcessedFrames;

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkMotionDetector::IdentifyInputArrays(vtkPolyData* polydata)
{
  // Get lidar vendor if first frame
  bool valid = true;
  if (this->NbProcessedFrames == 0)
  {
    std::string vendorName = UNKNOWN_NAME;
    if (polydata->GetFieldData()->HasArray("Vendor"))
    {
      auto vendorArray =
        dynamic_cast<vtkStringArray*>(polydata->GetFieldData()->GetAbstractArray("Vendor"));
      vendorName = vendorArray->GetValue(0);
      if (vendorName != LIVOX_NAME && vendorName != VELODYNE_NAME && vendorName != HESAI_NAME)
      {
        vendorName = UNKNOWN_NAME;
      }
    }
    this->Internals->Lidar = this->Internals->LidarVendors.at(vendorName);
    vtkLog(INFO, "Lidar vendor is identified as " << vendorName << "\n");
  }

  // Check if the requested arrays exist or not
  switch (this->Internals->Lidar)
  {
    case vtkInternals::LidarVendor::VELODYNE:
    {
      valid = polydata->GetPointData()->HasArray("azimuth") &&
        polydata->GetPointData()->HasArray("vertical_angle") &&
        polydata->GetPointData()->HasArray("distance_m");
      break;
    }
    case vtkInternals::LidarVendor::LIVOX:
    case vtkInternals::LidarVendor::HESAI:
    {
      valid = polydata->GetPointData()->HasArray("distance_m");
      break;
    }
    default:
    {
      break;
    }
  }
  return valid;
}
