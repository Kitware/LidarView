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
#include <vtkPolyDataWriter.h>
#include <vtkPolyLine.h>
#include <vtkQuaternion.h>
#include <vtkRadiusOutlierRemoval.h>
#include <vtkRemovePolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkStringArray.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkXMLPolyDataReader.h>

// STD
#include <iomanip>
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
   * When a new point is added, the sigma and the mean are updated
   */
  struct Gaussian1D
  {
    // Constructor
    Gaussian1D() = default;
    Gaussian1D(double mean,
      double sigma,
      int maxTTL,
      unsigned int nb = 1,
      bool isMotion = false,
      double weight = 1.0)
      : Mean(mean)
      , Sigma(sigma)
      , MaxTTL(maxTTL)
      , TTL(maxTTL)
      , NbInliers(nb)
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
    unsigned int NbInliers = 0;

    // If or not the gaussian cluster represent a motion area
    bool IsMotion = false;

    // Weight of the distribution
    double Weight = 1.0;

    // Get normal proba of point x
    double operator()(double x) const
    {
      return 1.0 / (this->Sigma * sqrt(2 * vtkMath::Pi())) *
        exp(-std::pow(x - this->Mean, 2) / (2.0 * std::pow(this->Sigma, 2)));
    }

    // Get weighted proba of point x
    [[nodiscard]] double ComputeWeightedProba(double x) const
    {
      return this->Weight / (this->Sigma * sqrt(2 * vtkMath::Pi())) *
        exp(-std::pow(x - this->Mean, 2) / (2.0 * std::pow(this->Sigma, 2)));
    }

    // Update Gaussian parameters using new value and its associated weight
    void UpdateParams(double x, double weightX)
    {
      // Update the weight
      double oldWeight = this->Weight;
      double sumWeight = this->NbInliers * oldWeight;
      this->Weight = (sumWeight + weightX) / (this->NbInliers + 1);

      // Update the mean
      double oldMean = this->Mean;
      this->Mean = oldMean + weightX * (x - oldMean) / (sumWeight + weightX);

      // Update the standard deviation using Welford's method
      this->Sigma = std::sqrt(
        (sumWeight * std::pow(this->Sigma, 2) + weightX * (x - oldMean) * (x - this->Mean)) /
        (sumWeight + weightX));

      // Update the number of sample
      ++this->NbInliers;
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
  class GaussianMixture1D
  {
  public:
    // Default constructor
    GaussianMixture1D() = default;

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
      for (auto it = this->Gaussians.begin(); it != this->Gaussians.end(); ++it)
      {
        probas.emplace_back(it->ComputeWeightedProba(x));
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
      for (auto it = this->Gaussians.begin(); it != this->Gaussians.end(); ++it)
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
        Gaussian1D newGaussian(x, 0.2, this->MaxTTL, 1, motionEstim);
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
      Gaussian1D newGaussian(x, 0.2, this->MaxTTL, this->ItClosest->NbInliers + 1, motionEstim);
      probas.clear();
      for (auto& gaussian : this->Gaussians)
      {
        probas.emplace_back(gaussian(x));
      }
      probas.emplace_back(newGaussian(x));
      double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);
      int idProba = 0;
      // Update existing gaussians with new weights
      for (auto& gaussian : this->Gaussians)
      {
        gaussian.UpdateParams(x, probas[idProba] / sumProba);
        ++idProba;
      }
      // Set weight for new gaussian and add it to the model
      double newGaussianWeight = (probas.back() / sumProba) / (this->ItClosest->NbInliers + 1);
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
      auto it = this->Gaussians.begin();
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

    // private:
    // Maximum number of frames for TTL
    int MaxTTL = 50;

    // Iterator to the cluster which the mean value is the closest to the point
    std::list<Gaussian1D>::iterator ItClosest;

    // Gaussian distributions
    std::list<Gaussian1D> Gaussians;
  };

  // The spherical depth map with gaussian mixture model
  std::vector<GaussianMixture1D> Map;

  /**
   * Spherical map bounds in degrees. They depend on lidar model
   * VLP-16 :      Vertical ~[-15, 15]  Azimuth ~[0, 360]
   * VLP-32c:      Vertical ~[-25, 15]  Azimuth ~[0, 360]
   * Livox HAP   : Vertical ~[70, 105] Azimuth ~[-60, 60]
   * Livox MID360: Vertical ~[30, 100] Azimuth ~[-180, 180]
   * */
  double VerticalBounds[2] = { -90., 90. };
  double AzimuthBounds[2] = { -180., 180. };

  /*
   * Resolution in degrees. Depend on lidar resolution
   */
  double VerticalResolution = 1.;
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

  // Number of processed frames
  int NbProcessedFrames = 0;

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
        Eigen::Vector3d point;
        this->AzimuthBounds[0] = DBL_MAX;
        this->AzimuthBounds[1] = -DBL_MAX;
        this->VerticalBounds[0] = DBL_MAX;
        this->VerticalBounds[1] = -DBL_MAX;
        for (auto id = 0; id < polydata->GetNumberOfPoints(); ++id)
        {
          polydata->GetPoint(id, point.data());
          double r = polydata->GetPointData()->GetArray("distance_m")->GetTuple1(id);
          double azimuth = vtkMath::DegreesFromRadians(std::atan2(point.y(), point.x()));
          double vertical = vtkMath::DegreesFromRadians(std::acos(point.z() / r));
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
      "The azimuth  angle range is [ " << this->AzimuthBounds[0] << ", " << this->AzimuthBounds[1]
                                       << "]");
    vtkLog(INFO,
      "The vertical angle range is [ " << this->VerticalBounds[0] << ", " << this->VerticalBounds[1]
                                       << "]");
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
    this->NbProcessedFrames = 0;
  }

  /*
   * Convert cartesian coordinates of point x into its spherical coordinates
   */
  Eigen::Vector3d ToSpherical(const Eigen::Vector3d& pt)
  {
    if (pt.norm() <= 1e-6)
    {
      return { 0., 0., 0. };
    }

    Eigen::Vector3d sphericalCoords;
    // Compute the radius
    sphericalCoords(0) = pt.norm();
    // Compute the vertical angle
    sphericalCoords(1) = vtkMath::DegreesFromRadians(std::acos(std::min(1., pt.z() / pt.norm())));
    // Compute the horizontal angle (azimuth)
    sphericalCoords(2) = vtkMath::DegreesFromRadians(std::atan2(pt.y(), pt.x()));
    return sphericalCoords;
  }

  /*
   * Update the TTL of the gaussian mixture model
   */
  void UpdateTTL()
  {
    for (auto& mixture : this->Map)
    {
      mixture.UpdateTTL();
    }
  }
};

class vtkMotionDetector::vtkClustering
{
public:
  /**
   * @brief This gaussian struct is used to compute the multivariate gaussian distribution:
   * 1.0 / sqrt(2 * pi^k) * cov.determinant) * exp(- 1 / 2 * (pt - mean) * cov.inverse * (pt -
   * mean).transpose) When a new point is added, the covariance and the mean are updated
   */
  struct Gaussian3D
  {
    // Constructor
    Gaussian3D() = default;
    Gaussian3D(Eigen::Vector3d mean,
      Eigen::Matrix3d cov,
      int maxTTL,
      int id = 0,
      unsigned int nb = 1,
      double weight = 1.0)
      : Mean(mean)
      , Cov(cov)
      , MaxTTL(maxTTL)
      , TTL(maxTTL)
      , Id(id)
      , NbInliers(nb)
      , Weight(weight){};

    // Mean of the gaussian
    Eigen::Vector3d Mean = { 0., 0., 0. };

    // Standard deviation of the gaussian
    Eigen::Matrix3d Cov = Eigen::Matrix3d::Identity();

    // Maximum number of frames for TTL
    int MaxTTL = 50;

    // Time to live of the gaussian
    int TTL = 50;

    // Index of the cluster in current frame
    int Id = 0;

    // Number of samples added into gaussian
    unsigned int NbInliers = 0;

    // Weight of the distribution
    double Weight = 1.0;

    std::vector<int> PointsId;

    // Get normal proba of point pt
    double operator()(const Eigen::Vector3d& pt) const
    {
      return 1.0 / std::sqrt(this->Cov.determinant() * std::pow(2 * vtkMath::Pi(), 3)) *
        exp(-0.5 * (pt - this->Mean).transpose() * this->Cov.inverse() * (pt - this->Mean));
    }

    // Get weighted proba of point pt
    [[nodiscard]] double ComputeWeightedProba(const Eigen::Vector3d& pt) const
    {
      return this->Weight / std::sqrt(this->Cov.determinant() * std::pow(2 * vtkMath::Pi(), 3)) *
        exp(-0.5 * (pt - this->Mean).transpose() * this->Cov.inverse() * (pt - this->Mean));
    }

    // Update parameters using new value and the weight of this value
    void UpdateParams(const Eigen::Vector3d& pt, double weightX)
    {
      // Update the weight
      double oldWeight = this->Weight;
      double sumWeight = this->NbInliers * oldWeight;
      this->Weight = (sumWeight + weightX) / (this->NbInliers + 1);

      // Update the mean
      Eigen::Vector3d oldMean = this->Mean;
      this->Mean = oldMean + weightX * (pt - oldMean) / (sumWeight + weightX);

      // Update the covariance using Welford's method
      // For the moment, do not update covariance for the clustering and tracking
      // to keep the cluster size
      // this->Cov =
      //   (sumWeight * this->Cov + weightX * (pt - oldMean) * (pt - this->Mean).transpose()) /
      //   (sumWeight + weightX);

      // Update the number of sample
      ++NbInliers;
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
   * @brief The GaussianMixture class in vtkClustering class constructs a gaussian mixture model
   * to extract cluster of detected motion points and track cluster frame by frame
   * Each gaussian distribution represents a cluster with a weight and a cluster id label
   */
  class GaussianMixture3D
  {
  public:
    // Default constructor
    GaussianMixture3D() = default;

    // Clear all gaussians distribution in GMM
    void Reset()
    {
      this->Gaussians.clear();
      this->UniqueID = 0;
    };

    void SetMaxTTL(int maxTTL)
    {
      if (this->MaxTTL != maxTTL)
      {
        this->MaxTTL = maxTTL;
        for (auto& gaussian : this->Gaussians)
          gaussian.MaxTTL = this->MaxTTL;
      }
    }

    [[nodiscard]] int GetMaxTTL() const { return this->MaxTTL; }

    // Set value to intialiaze covariance with the required cluster size
    void SetCovCoeff(double clusterSize) { this->CovCoeff = std::pow(clusterSize, 2.); }

    void SetClusterRadius(double clusterSize) { this->ClusterRadius = clusterSize; }
    void SetMinClusterPoints(int minNumber) { this->MinClusterPoints = minNumber; }

    // Increment Unique ID Counter
    int GetNewUniqueID()
    {
      ++this->UniqueID;
      return this->UniqueID - 1;
    }

    // Getter to obtain points indices and cluster id label of extracted clusters for a frame
    void GetClusters(std::vector<std::vector<int>>& clusters, std::vector<int>& clusterId)
    {
      for (auto& gaussian : this->Gaussians)
      {
        if (gaussian.PointsId.empty())
          continue;
        clusters.emplace_back(gaussian.PointsId);
        clusterId.emplace_back(gaussian.Id);
      }
    }

    // Reset cluster PointsId to store new clusters for next frame
    void ClearClusters()
    {
      for (auto& gaussian : this->Gaussians)
      {
        gaussian.PointsId.clear();
      }
    }

    /**
     * @brief Add the new point into gaussian mixture model
     * If the model is empty, add a new cluster with weight = 1
     * If the point is too far from existing clusters, add a new cluster with appropriate weight
     * Otherwise, add the point into model and update parameters
     */
    void AddPoint(const Eigen::Vector3d& pt, int ptId)
    {
      // Create a new gaussian if the gaussian mixture model is empty
      if (this->Gaussians.empty())
      {
        Eigen::Matrix3d covInit = this->CovCoeff * Eigen::Matrix3d::Identity();
        Gaussian3D newGaussian(pt, covInit, this->MaxTTL, this->GetNewUniqueID());
        newGaussian.PointsId.emplace_back(ptId);
        this->Gaussians.push_back(newGaussian);
        this->ItClosest = this->Gaussians.begin();
        return;
      }
      // Find the closest gaussian for the new point
      std::vector<double> probas;
      double minDistance = FLT_MAX;
      this->ItClosest = this->Gaussians.begin();
      for (auto it = this->Gaussians.begin(); it != this->Gaussians.end(); ++it)
      {
        probas.emplace_back(it->ComputeWeightedProba(pt));
        double dist = (pt - it->Mean).norm();
        if (dist < minDistance)
        {
          minDistance = dist;
          this->ItClosest = it;
        }
      }

      // Update current gaussian mixture model if the new point is close to a gaussian cluster
      Eigen::Vector3d distance = pt - this->ItClosest->Mean;
      if (distance.norm() < 3. * this->ClusterRadius)
      {
        this->ItClosest->PointsId.emplace_back(ptId);
        int idProba = 0;
        double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);
        for (auto& gaussian : this->Gaussians)
        {
          gaussian.UpdateParams(pt, probas[idProba] / sumProba);
          ++idProba;
        }
        this->ResetTTL();
        return;
      }

      // Create a new gaussian if the new point is far from existing distributions
      Eigen::Matrix3d covInit = this->CovCoeff * Eigen::Matrix3d::Identity();
      Gaussian3D newGaussian(
        pt, covInit, this->MaxTTL, this->GetNewUniqueID(), this->ItClosest->NbInliers + 1);
      newGaussian.PointsId.emplace_back(ptId);
      probas.clear();
      for (auto gaussian : this->Gaussians)
      {
        probas.emplace_back(gaussian(pt));
      }
      probas.emplace_back(newGaussian(pt));
      double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);
      int idProba = 0;
      // Update existing gaussians with new weights
      for (auto& gaussian : this->Gaussians)
      {
        gaussian.UpdateParams(pt, probas[idProba] / sumProba);
        ++idProba;
      }
      // Set weight for new gaussian and add it to the model
      double newGaussianWeight = (probas.back() / sumProba) / (this->ItClosest->NbInliers + 1);
      newGaussian.Weight = newGaussianWeight;
      this->Gaussians.push_back(newGaussian);
      this->ItClosest = std::prev(this->Gaussians.end());
    }

    /**
     * @brief Update the time to live of the each gaussian in the model
     * If a gaussian is dead, erase it. Then the weight of the remains
     * gaussians need to be normalized and the motion labels need to be updated
     */
    void UpdateClusters()
    {
      if (this->Gaussians.empty())
        return;
      auto it = this->Gaussians.begin();
      while (it != this->Gaussians.end())
      {
        // Check if or not the gaussian is still alive
        bool stillAlive = it->UpdateTTL();

        // Erase the gaussian if it is not alive
        if (!stillAlive || int(it->PointsId.size()) < this->MinClusterPoints)
        {
          // erase current iterator and get next on
          it = this->Gaussians.erase(it);
          // no need to increment iterator here
        }
        else
        {
          ++it; // increment iterator here
        }
      }
      // Normalise weights and clear points id
      int nbGaussians = this->Gaussians.size();
      for (auto& gaussian : this->Gaussians)
      {
        gaussian.NbInliers = 1;
        gaussian.Weight = 1. / double(nbGaussians);
      }
    }

    /**
     * @brief Reset time to live value to its maximum when a new
     * point is added to a gaussian distribution
     */
    void ResetTTL() { this->ItClosest->TTL = this->MaxTTL; }

  private:
    // counter for a new cluster ID
    int UniqueID = 0;

    // Maximum number of frames for TTL
    int MaxTTL = 10;

    // To initialiaze covariance with 0.2 research radius
    double CovCoeff = 0.04;

    // Minimum points to form a cluster
    int MinClusterPoints = 10;

    // Radius to search for cluster
    double ClusterRadius = 0.5;

    // Iterator to the cluster whose mean value is the closest to the point
    std::list<Gaussian3D>::iterator ItClosest;

    // Gaussian distributions
    std::list<Gaussian3D> Gaussians;
  };

  GaussianMixture3D Clusters;
};

constexpr unsigned int LIDAR_FRAME_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int MOTION_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int CLUSTERS_OUTPUT_PORT = 1;
constexpr unsigned int CLUSTERS_TEXT_OUTPUT_PORT = 2;
constexpr unsigned int OUTPUT_PORT_COUNT = 3;

// Implementation of the New function
vtkStandardNewMacro(vtkMotionDetector)

//----------------------------------------------------------------------------
vtkMotionDetector::vtkMotionDetector()
  : Internals(new vtkMotionDetector::vtkInternals())
  , Clustering(new vtkMotionDetector::vtkClustering())
{
  // One input port
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);

  // The accumulation of stabilized frames
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);

  // Initialize the internal parameters
  this->Internals->ResetMap();
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
  if (port == CLUSTERS_TEXT_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
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
  this->Internals->NbProcessedFrames = 0;
  if (this->ClusterExtractor == static_cast<int>(vtkMotionDetector::Extractor::GMM))
  {
    this->Clustering->Clusters.Reset();
  }
  if (this->ClusterExtractor == static_cast<int>(vtkMotionDetector::Extractor::REGION_GROWING))
  {
    this->ClustersGrid.VoxelMap.clear();
    this->ClustersGrid.BackgroudMap.clear();
    this->NewClusterIdx = 0;
  }
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
void vtkMotionDetector::SetDetectionRange(double minDist, double maxDist)
{
  if (this->DetectionRange[0] != minDist || this->DetectionRange[1] != maxDist)
  {
    this->DetectionRange[0] = minDist;
    this->DetectionRange[1] = maxDist;
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::SetClusterRadius(double radius)
{
  if (this->ClusterRadius != radius)
  {
    this->ClusterRadius = radius;
  }
  if (this->ClusterExtractor == static_cast<int>(vtkMotionDetector::Extractor::GMM))
  {
    this->Clustering->Clusters.SetClusterRadius(radius);
    this->Clustering->Clusters.SetCovCoeff(radius);
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::SetClusterMinNbPoints(int minNbPoints)
{
  if (this->ClusterMinNbPoints != minNbPoints)
  {
    this->ClusterMinNbPoints = minNbPoints;
  }
  if (this->ClusterExtractor == static_cast<int>(vtkMotionDetector::Extractor::GMM))
  {
    this->Clustering->Clusters.SetMinClusterPoints(minNbPoints);
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::SetTrackingWindowSizes(int trackingWindowSizes)
{
  if (this->TrackingWindowSizes != trackingWindowSizes)
  {
    this->TrackingWindowSizes = trackingWindowSizes;
    if (this->ClusterExtractor == static_cast<int>(vtkMotionDetector::Extractor::GMM))
    {
      this->Clustering->Clusters.SetMaxTTL(trackingWindowSizes);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::SetClusterGridResolution(float resolution)
{
  if (resolution <= 0)
  {
    vtkWarningMacro(<< "Clustering grid resolution can not be 0 or negative!");
    return;
  }
  if (this->ClusterGridResolution != resolution)
  {
    this->ClusterGridResolution = resolution;
    this->ClustersGrid.VoxelMap.clear();
    for (int i = 0; i < 3; i++)
    {
      this->ClustersGrid.GridSize[i] = this->ClustersGrid.GridSize[i] *
        this->ClustersGrid.Resolution / this->ClusterGridResolution;
    }
    this->ClustersGrid.Resolution = this->ClusterGridResolution;
  }
}

//----------------------------------------------------------------------------
void vtkMotionDetector::EstimateMotion(vtkSmartPointer<vtkPolyData> polydata,
  vtkSmartPointer<vtkPolyData> motionPolydata)
{
  vtkNew<vtkPoints> motionPoints;
  motionPolydata->SetPoints(motionPoints);
  // Useful array for motionPolydata
  vtkSmartPointer<vtkDoubleArray> motionDistance = vtkSmartPointer<vtkDoubleArray>::New();
  motionDistance->SetName("distance_m");
  vtkSmartPointer<vtkDoubleArray> motionIntensity = vtkSmartPointer<vtkDoubleArray>::New();
  motionIntensity->SetName("intensity");

  // If subsample resolution is not 0, storage motion points near to lidar to another motion
  // polydata and subsample the near motion points
  vtkNew<vtkAppendPolyData> appendPoints;
  // Motion points near to lidar
  vtkNew<vtkPolyData> motionPolydataNear;
  vtkNew<vtkPoints> motionPointsNear;
  motionPolydataNear->SetPoints(motionPointsNear);
  vtkSmartPointer<vtkDoubleArray> motionDistanceNear = vtkSmartPointer<vtkDoubleArray>::New();
  motionDistanceNear->SetName("distance_m");
  vtkSmartPointer<vtkDoubleArray> motionIntensityNear = vtkSmartPointer<vtkDoubleArray>::New();
  motionIntensityNear->SetName("intensity");

  Eigen::Vector3d point;
  Eigen::Vector3d sphericalPoint;
  for (vtkIdType id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    // Get point and compute its spherical coordinates
    polydata->GetPoint(id, point.data());
    switch (this->Internals->Lidar)
    {
      case vtkInternals::LidarVendor::VELODYNE:
      {
        sphericalPoint(0) = polydata->GetPointData()->GetArray("distance_m")->GetTuple1(id);
        sphericalPoint(1) = polydata->GetPointData()->GetArray("azimuth")->GetTuple1(id) / 100.;
        sphericalPoint(2) = polydata->GetPointData()->GetArray("vertical_angle")->GetTuple1(id);
        break;
      }
      case vtkInternals::LidarVendor::LIVOX:
      case vtkInternals::LidarVendor::HESAI:
      {
        sphericalPoint(0) = polydata->GetPointData()->GetArray("distance_m")->GetTuple1(id);
        sphericalPoint(1) = vtkMath::DegreesFromRadians(std::atan2(point.y(), point.x()));
        sphericalPoint(2) = vtkMath::DegreesFromRadians(std::acos(point.z() / sphericalPoint(0)));
        break;
      }
      default:
      {
        sphericalPoint = this->Internals->ToSpherical(point);
        break;
      }
    }

    // Check whether or not the point is in the detection range
    if (sphericalPoint(0) < this->DetectionRange[0] || sphericalPoint(0) > this->DetectionRange[1])
    {
      continue;
    }

    bool hasMoved = false;
    // Convert spherical coordinates to spherical map coordinates
    unsigned int idxVertical = (sphericalPoint(2) - this->Internals->VerticalBounds[0]) /
      this->Internals->VerticalResolution;
    unsigned int idxAzimuth =
      (sphericalPoint(1) - this->Internals->AzimuthBounds[0]) / this->Internals->AzimuthResolution;
    idxVertical = std::min(idxVertical, this->Internals->NbVertical - 1);
    idxAzimuth = std::min(idxAzimuth, this->Internals->NbAzimuth - 1);

    // Evaluate the mixture model on the current data
    // It returns the motion probability of the point
    // and stores the iterator of the cluster which give the max proba
    std::vector<double> probas;
    int idx1d = idxAzimuth + this->Internals->NbAzimuth * idxVertical;
    this->Internals->Map[idx1d].Evaluate(
      sphericalPoint(0), this->Internals->NbProcessedFrames < this->InitNbFrames, probas, hasMoved);

    // Add the depth to the correct "pixel" and update parameters of the model
    this->Internals->Map[idx1d].AddPoint(sphericalPoint(0), hasMoved, probas);
    if (hasMoved)
    {
      // If subsample near motion points is required, put near motion points into motionPolydataNear
      if (this->SubsampleResolution > 0 && sphericalPoint(0) < this->SubsampleRange)
      {
        motionPointsNear->InsertNextPoint(point.data());
        motionDistanceNear->InsertNextTuple1(sphericalPoint(0));
        motionIntensityNear->InsertNextTuple1(
          polydata->GetPointData()->GetArray("intensity")->GetTuple1(id));
      }
      else
      {
        motionPoints->InsertNextPoint(point.data());
        motionDistance->InsertNextTuple1(sphericalPoint(0));
        motionIntensity->InsertNextTuple1(
          polydata->GetPointData()->GetArray("intensity")->GetTuple1(id));
      }
    }
  }

  // Update time to live of the gaussians
  this->Internals->UpdateTTL();

  if (motionPolydata->GetNumberOfPoints() == 0 && motionPolydataNear->GetNumberOfPoints() == 0)
    return;

  // Subsample motion points and remove outliers
  // Add arrays and generate vertices for motion polydata
  motionPolydata->GetPointData()->AddArray(motionDistance);
  motionPolydata->GetPointData()->AddArray(motionIntensity);
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(motionPolydata->GetNumberOfPoints());
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(1, connectivity);
  motionPolydata->SetVerts(cellArray);
  for (vtkIdType k = 0; k < motionPolydata->GetNumberOfPoints(); ++k)
  {
    connectivity->SetValue(k, k);
  }
  // Subsample motion points if required
  if (this->SubsampleResolution > 0 && motionPolydataNear->GetNumberOfPoints() > 0)
  {
    // Add arrays and generate vertices for motion polydata
    motionPolydataNear->GetPointData()->AddArray(motionDistanceNear);
    motionPolydataNear->GetPointData()->AddArray(motionIntensityNear);
    vtkNew<vtkIdTypeArray> connectivityNear;
    connectivityNear->SetNumberOfValues(motionPolydataNear->GetNumberOfPoints());
    vtkNew<vtkCellArray> cellArrayNear;
    cellArrayNear->SetData(1, connectivityNear);
    motionPolydataNear->SetVerts(cellArrayNear);
    for (vtkIdType k = 0; k < motionPolydataNear->GetNumberOfPoints(); ++k)
    {
      connectivityNear->SetValue(k, k);
    }
    // Subsample near motion points
    vtkNew<vtkCleanPolyData> cleanPolyData;
    cleanPolyData->SetInputData(motionPolydataNear);
    cleanPolyData->SetAbsoluteTolerance(this->SubsampleResolution);
    cleanPolyData->SetToleranceIsAbsolute(true);
    cleanPolyData->Update();
    motionPolydataNear->ShallowCopy(cleanPolyData->GetOutput());
  }

  // Append near and far motion points
  if (motionPolydataNear->GetNumberOfPoints() > 0)
  {
    appendPoints->AddInputData(motionPolydataNear);
    appendPoints->AddInputData(motionPolydata);
    appendPoints->Update();
    motionPolydata->ShallowCopy(appendPoints->GetOutput());
  }

  if (motionPolydata->GetNumberOfPoints() == 0)
  {
    return;
  }

  // Remove outlier
  vtkNew<vtkRadiusOutlierRemoval> removal;
  removal->SetInputData(motionPolydata);
  removal->SetRadius(this->RemovalOutlierRadius);
  removal->SetNumberOfNeighbors(this->RemovalOutlierNeighbors);
  removal->GenerateVerticesOn();
  removal->Update();
  // Update output motion polydata with filtered motion points
  motionPolydata->ShallowCopy(removal->GetOutput());
}

//-----------------------------------------------------------------------------
void vtkMotionDetector::ExtractClustersWithEuclidean(vtkSmartPointer<vtkPolyData> input,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput)
{
  if (input->GetNumberOfPoints() == 0)
  {
    vtkLog(INFO, "Not enough motion points: clusters not extracted");
    return;
  }

  // Extract cluster
  vtkNew<vtkEuclideanClusterExtraction> cluster;
  cluster->SetInputData(input);
  cluster->SetExtractionModeToAllClusters();
  cluster->SetRadius(this->ClusterRadius);
  cluster->ColorClustersOn();
  cluster->Update();

  // Create output with vertices
  input->ShallowCopy(cluster->GetOutput());
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(input->GetNumberOfPoints());
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(1, connectivity);
  input->SetVerts(cellArray);
  for (vtkIdType k = 0; k < input->GetNumberOfPoints(); ++k)
  {
    connectivity->SetValue(k, k);
  }

  // Get point indices of each cluster
  int numClusters = cluster->GetNumberOfExtractedClusters();
  std::unordered_map<int, std::vector<int>> clusters;
  clusters.reserve(numClusters);
  for (int clusterId = 0; clusterId < numClusters; ++clusterId)
  {
    for (vtkIdType pointId = 0; pointId < input->GetNumberOfPoints(); ++pointId)
    {
      auto currClusterId = input->GetPointData()->GetArray("ClusterId")->GetTuple1(pointId);
      clusters[currClusterId].emplace_back(pointId);
    }
  }

  // Compute cluster stats: size, mean depth, mean intensity etc
  this->ClustersStats.clear();
  for (const auto& clus : clusters)
  {
    auto& clusterId = clus.first;  // shortcut for clusterId
    auto& ptIndices = clus.second; // shortcut for point indices
    int nbClusterPoints = ptIndices.size();
    if (nbClusterPoints < this->ClusterMinNbPoints)
    {
      for (const auto& pointId : ptIndices)
      {
        input->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, -1);
      }
      continue;
    }

    // Prepare data for PCA
    vtkNew<vtkPoints> clusterPoints;
    vtkNew<vtkPolyData> clusterPointsPolydata;
    clusterPointsPolydata->SetPoints(clusterPoints);
    vtkNew<vtkDoubleArray> xArray;
    xArray->SetNumberOfComponents(1);
    xArray->SetName("x");
    vtkNew<vtkDoubleArray> yArray;
    yArray->SetNumberOfComponents(1);
    yArray->SetName("y");
    vtkNew<vtkDoubleArray> zArray;
    zArray->SetNumberOfComponents(1);
    zArray->SetName("z");

    ClusterStats clusterInfo;
    // Calculate the average depth value and bounding box for this cluster
    double depth = 0.0;
    double intensity = 0.0;
    for (const auto& pointId : ptIndices)
    {
      depth += input->GetPointData()->GetArray("distance_m")->GetTuple1(pointId);
      intensity += input->GetPointData()->GetArray("intensity")->GetTuple1(pointId);
      Eigen::Vector3d point;
      input->GetPoint(pointId, point.data());
      xArray->InsertNextValue(point.x());
      yArray->InsertNextValue(point.y());
      zArray->InsertNextValue(point.z());
      clusterPoints->InsertNextPoint(point.data());
    }
    // For PCA
    vtkNew<vtkTable> datasetTable;
    datasetTable->AddColumn(xArray);
    datasetTable->AddColumn(yArray);
    datasetTable->AddColumn(zArray);
    vtkNew<vtkPCAStatistics> pcaStatistics;
    pcaStatistics->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, datasetTable);
    pcaStatistics->SetColumnStatus("x", 1);
    pcaStatistics->SetColumnStatus("y", 1);
    pcaStatistics->SetColumnStatus("z", 1);
    pcaStatistics->RequestSelectedColumns();
    pcaStatistics->SetDeriveOption(true);
    pcaStatistics->Update();
    vtkNew<vtkDoubleArray> eigenvectors;
    pcaStatistics->GetEigenvectors(eigenvectors);

    Eigen::Vector3d pointcloudCentroid;
    vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter = vtkSmartPointer<vtkCenterOfMass>::New();
    centerOfMassFilter->SetInputData(clusterPointsPolydata);
    centerOfMassFilter->SetUseScalarsAsWeights(false);
    centerOfMassFilter->Update();
    centerOfMassFilter->GetCenter(pointcloudCentroid.data());

    // clang-format off
    double matrix[16] = {
      eigenvectors->GetValue(6), eigenvectors->GetValue(3), eigenvectors->GetValue(0), pointcloudCentroid.x(),
      eigenvectors->GetValue(7), eigenvectors->GetValue(4), eigenvectors->GetValue(1), pointcloudCentroid.y(),
      eigenvectors->GetValue(8), eigenvectors->GetValue(5), eigenvectors->GetValue(2), pointcloudCentroid.z(),
      0., 0., 0., 1. };
    // clang-format on
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(matrix);

    // Transform cluster point cloud
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    transformFilter->SetTransform(transform->GetInverse());
    transformFilter->SetInputData(clusterPointsPolydata);
    transformFilter->Update();
    double bounds[6];
    transformFilter->GetOutput()->GetBounds(bounds);

    clusterInfo.ClusterId = clusterId;
    clusterInfo.NbPoints = nbClusterPoints;
    depth /= nbClusterPoints;
    intensity /= nbClusterPoints;
    clusterInfo.MeanDepth = depth;
    clusterInfo.MeanIntensity = intensity;
    clusterInfo.BoundingBox.SetTransform(matrix);
    clusterInfo.BoundingBox.SetVertices(
      bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
    this->ClustersStats.emplace_back(clusterInfo);
  }
  // Sort clusters based on their average depth values
  std::sort(this->ClustersStats.begin(),
    this->ClustersStats.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.MeanDepth < cluster2.MeanDepth; });
  std::vector<int> newClusterIds(numClusters);
  int newClusterId = 0;
  for (auto& cluster : this->ClustersStats)
  {
    newClusterIds[cluster.ClusterId] = newClusterId;
    ++newClusterId;
  }

  // Reset cluster id for points and cluster stats
  newClusterId = 0;
  for (auto& cluster : this->ClustersStats)
  {
    cluster.ClusterId = newClusterId;
    ++newClusterId;
  }
  auto clusterIdArray = input->GetPointData()->GetArray("ClusterId");
  for (vtkIdType pointId = 0; pointId < input->GetNumberOfPoints(); pointId++)
  {
    auto clusterId = clusterIdArray->GetTuple1(pointId);
    if (clusterId < 0)
      continue;
    input->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, newClusterIds[clusterId]);
  }

  // Label cluster by geometry dimension
  for (auto& cluster : this->ClustersStats)
  {
    if (cluster.BoundingBox.GetSize().z() >= 0.8 && cluster.BoundingBox.GetSize().z() <= 2.2 &&
      ((cluster.BoundingBox.GetSize().x() >= 0.2 && cluster.BoundingBox.GetSize().x() <= 0.7) ||
        (cluster.BoundingBox.GetSize().y() >= 0.2 && cluster.BoundingBox.GetSize().y() <= 0.7)))
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
  for (const auto& cluster : this->ClustersStats)
  {
    vtkNew<vtkCubeSource> cubeSource;
    vtkNew<vtkPolyData> source;
    cubeSource->SetBounds(cluster.BoundingBox.GetVertices().data());
    cubeSource->SetCenter(cluster.BoundingBox.GetCenter().data());
    // Transform bounding box
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(cluster.BoundingBox.GetTransform().data());
    transformFilter->SetTransform(transform);
    transformFilter->SetInputData(source);
    transformFilter->SetInputConnection(cubeSource->GetOutputPort());
    transformFilter->Update();
    source->ShallowCopy(transformFilter->GetOutput());

    std::string blockName("Cluster-" + std::to_string(cluster.ClusterId));
    clustersOutput->SetBlock(blockId, source);
    clustersOutput->GetMetaData(blockId)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());

    // Create field data and add information to it
    vtkSmartPointer<vtkIntArray> bboxId = vtkSmartPointer<vtkIntArray>::New();
    bboxId->SetName("ClusterId");
    bboxId->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxDistances = vtkSmartPointer<vtkFloatArray>::New();
    bboxDistances->SetName("Distance");
    bboxDistances->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxSizes = vtkSmartPointer<vtkFloatArray>::New();
    bboxSizes->SetName("Size");
    bboxSizes->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxCenters = vtkSmartPointer<vtkFloatArray>::New();
    bboxCenters->SetName("Center");
    bboxCenters->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxOrientations = vtkSmartPointer<vtkFloatArray>::New();
    bboxOrientations->SetName("Orientation");
    bboxOrientations->SetNumberOfComponents(4);
    vtkSmartPointer<vtkUnsignedShortArray> bboxLabels =
      vtkSmartPointer<vtkUnsignedShortArray>::New();
    bboxLabels->SetName("Label");
    bboxLabels->SetNumberOfComponents(1);

    vtkSmartPointer<vtkFieldData> fieldData = vtkSmartPointer<vtkFieldData>::New();
    fieldData->AddArray(bboxId);
    fieldData->AddArray(bboxDistances);
    fieldData->AddArray(bboxSizes);
    fieldData->AddArray(bboxCenters);
    fieldData->AddArray(bboxOrientations);
    fieldData->AddArray(bboxLabels);
    bboxId->InsertNextTuple1(static_cast<int>(cluster.ClusterId));
    bboxDistances->InsertNextTuple1(cluster.MeanDepth);
    bboxSizes->InsertNextTuple(cluster.BoundingBox.GetSize().data());
    bboxCenters->InsertNextTuple(cluster.BoundingBox.GetTrueCenter().data());
    bboxOrientations->InsertNextTuple(cluster.BoundingBox.GetOrientation().data());
    bboxLabels->InsertNextTuple1(static_cast<unsigned short>(cluster.ClusterLabel));

    clustersOutput->GetBlock(blockId)->SetFieldData(fieldData);

    ++blockId;
  }

  // Print clusters info
  // Set output for displaying clusters information
  vtkNew<vtkStringArray> data;
  data->SetName("Clusters Information");
  data->SetNumberOfComponents(1);
  std::ostringstream oss;
  for (const auto& cluster : this->ClustersStats)
  {
    oss << std::setprecision(3) << std::showpoint << "Cluster " << std::setw(2) << cluster.ClusterId
        << ": distance = " << std::setw(7) << cluster.MeanDepth << "m  "
        << "size = " << cluster.BoundingBox.GetSize().transpose() << "\n";
  }
  std::string clusterInfo = oss.str();
  data->InsertNextValue(clusterInfo.c_str());
  infoOutput->AddColumn(data);
}

//-----------------------------------------------------------------------------
void vtkMotionDetector::ExtractClustersWithGMM(vtkSmartPointer<vtkPolyData> input,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput)
{
  if (input->GetNumberOfPoints() == 0)
  {
    vtkLog(INFO, "Not enough motion points: clusters not extracted");
    return;
  }

  // Extract clusters with gaussian mixture model
  Eigen::Vector3d point;
  for (int id = 0; id < input->GetNumberOfPoints(); ++id)
  {
    input->GetPoint(id, point.data());
    this->Clustering->Clusters.AddPoint(point, id);
  }
  this->Clustering->Clusters.UpdateClusters();

  // Add array to store cluster Id
  // The array name is ClusterId to be same as the vtkEuclideanClusterExtraction filter
  vtkSmartPointer<vtkIntArray> pointLabel = vtkSmartPointer<vtkIntArray>::New();
  pointLabel->SetName("ClusterId");
  for (auto id = 0; id < input->GetNumberOfPoints(); ++id)
  {
    pointLabel->InsertNextTuple1(-1);
  }
  input->GetPointData()->AddArray(pointLabel);

  // Compute cluster stats: size, mean depth, mean intensity etc
  std::vector<std::vector<int>> clusters;
  std::vector<int> clusterUUID;
  this->Clustering->Clusters.GetClusters(clusters, clusterUUID);
  this->ClustersStats.clear();
  for (unsigned int id = 0; id < clusters.size(); ++id)
  {
    int nbClusterPoints = clusters[id].size();
    if (nbClusterPoints < this->ClusterMinNbPoints)
      continue;

    // Prepare data for PCA
    vtkNew<vtkPoints> clusterPoints;
    vtkNew<vtkPolyData> clusterPointsPolydata;
    clusterPointsPolydata->SetPoints(clusterPoints);
    vtkNew<vtkDoubleArray> xArray;
    xArray->SetNumberOfComponents(1);
    xArray->SetName("x");
    vtkNew<vtkDoubleArray> yArray;
    yArray->SetNumberOfComponents(1);
    yArray->SetName("y");
    vtkNew<vtkDoubleArray> zArray;
    zArray->SetNumberOfComponents(1);
    zArray->SetName("z");

    ClusterStats clusterInfo;
    // Calculate the average depth value and bounding box for this cluster
    double depth = 0.0;
    double intensity = 0.0;
    for (const auto& pointId : clusters[id])
    {
      depth += input->GetPointData()->GetArray("distance_m")->GetTuple1(pointId);
      intensity += input->GetPointData()->GetArray("intensity")->GetTuple1(pointId);
      input->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, clusterUUID[id]);
      Eigen::Vector3d point;
      input->GetPoint(pointId, point.data());
      xArray->InsertNextValue(point.x());
      yArray->InsertNextValue(point.y());
      zArray->InsertNextValue(point.z());
      clusterPoints->InsertNextPoint(point.data());
    }

    // For PCA
    vtkNew<vtkTable> datasetTable;
    datasetTable->AddColumn(xArray);
    datasetTable->AddColumn(yArray);
    datasetTable->AddColumn(zArray);
    vtkNew<vtkPCAStatistics> pcaStatistics;
    pcaStatistics->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, datasetTable);
    pcaStatistics->SetColumnStatus("x", 1);
    pcaStatistics->SetColumnStatus("y", 1);
    pcaStatistics->SetColumnStatus("z", 1);
    pcaStatistics->RequestSelectedColumns();
    pcaStatistics->SetDeriveOption(true);
    pcaStatistics->Update();
    vtkNew<vtkDoubleArray> eigenvectors;
    pcaStatistics->GetEigenvectors(eigenvectors);

    Eigen::Vector3d pointcloudCentroid;
    vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter = vtkSmartPointer<vtkCenterOfMass>::New();
    centerOfMassFilter->SetInputData(clusterPointsPolydata);
    centerOfMassFilter->SetUseScalarsAsWeights(false);
    centerOfMassFilter->Update();
    centerOfMassFilter->GetCenter(pointcloudCentroid.data());

    // clang-format off
    double matrix[16] = {
      eigenvectors->GetValue(6), eigenvectors->GetValue(3), eigenvectors->GetValue(0), pointcloudCentroid.x(),
      eigenvectors->GetValue(7), eigenvectors->GetValue(4), eigenvectors->GetValue(1), pointcloudCentroid.y(),
      eigenvectors->GetValue(8), eigenvectors->GetValue(5), eigenvectors->GetValue(2), pointcloudCentroid.z(),
      0., 0., 0., 1. };
    // clang-format on
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(matrix);

    // Transform cluster point cloud
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    transformFilter->SetTransform(transform->GetInverse());
    transformFilter->SetInputData(clusterPointsPolydata);
    transformFilter->Update();
    double bounds[6];
    transformFilter->GetOutput()->GetBounds(bounds);

    clusterInfo.ClusterId = clusterUUID[id];
    clusterInfo.NbPoints = nbClusterPoints;
    depth /= nbClusterPoints;
    intensity /= nbClusterPoints;
    clusterInfo.MeanDepth = depth;
    clusterInfo.MeanIntensity = intensity;
    clusterInfo.BoundingBox.SetTransform(matrix);
    clusterInfo.BoundingBox.SetVertices(
      bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
    this->ClustersStats.emplace_back(clusterInfo);
  }
  this->Clustering->Clusters.ClearClusters();

  // Label cluster by geometry dimension
  for (auto& cluster : this->ClustersStats)
  {
    if (cluster.BoundingBox.GetSize().z() >= 0.8 && cluster.BoundingBox.GetSize().z() <= 2.2 &&
      ((cluster.BoundingBox.GetSize().x() >= 0.2 && cluster.BoundingBox.GetSize().x() <= 0.7) ||
        (cluster.BoundingBox.GetSize().y() >= 0.2 && cluster.BoundingBox.GetSize().y() <= 0.7)))
    {
      cluster.ClusterLabel = vtkMotionDetector::Label::HUMAN;
    }
    else
    {
      cluster.ClusterLabel = vtkMotionDetector::Label::OTHERS;
    }
  }
  // Sort clusters based on their clusterId
  std::sort(this->ClustersStats.begin(),
    this->ClustersStats.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.ClusterId < cluster2.ClusterId; });

  // Add bounding box for each cluster into output
  int blockId = 0;
  for (const auto& cluster : this->ClustersStats)
  {
    vtkNew<vtkCubeSource> cubeSource;
    vtkNew<vtkPolyData> source;
    cubeSource->SetBounds(cluster.BoundingBox.GetVertices().data());
    cubeSource->SetCenter(cluster.BoundingBox.GetCenter().data());
    // Transform bounding box
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(cluster.BoundingBox.GetTransform().data());
    transformFilter->SetTransform(transform);
    transformFilter->SetInputData(source);
    transformFilter->SetInputConnection(cubeSource->GetOutputPort());
    transformFilter->Update();
    source->ShallowCopy(transformFilter->GetOutput());

    std::string blockName("Cluster-" + std::to_string(cluster.ClusterId));
    clustersOutput->SetBlock(blockId, source);
    clustersOutput->GetMetaData(blockId)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());

    // Create field data and add information to it
    vtkSmartPointer<vtkIntArray> bboxId = vtkSmartPointer<vtkIntArray>::New();
    bboxId->SetName("ClusterId");
    bboxId->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxDistances = vtkSmartPointer<vtkFloatArray>::New();
    bboxDistances->SetName("Distance");
    bboxDistances->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxSizes = vtkSmartPointer<vtkFloatArray>::New();
    bboxSizes->SetName("Size");
    bboxSizes->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxCenters = vtkSmartPointer<vtkFloatArray>::New();
    bboxCenters->SetName("Center");
    bboxCenters->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxOrientations = vtkSmartPointer<vtkFloatArray>::New();
    bboxOrientations->SetName("Orientation");
    bboxOrientations->SetNumberOfComponents(4);
    vtkSmartPointer<vtkUnsignedShortArray> bboxLabels =
      vtkSmartPointer<vtkUnsignedShortArray>::New();
    bboxLabels->SetName("Label");
    bboxLabels->SetNumberOfComponents(1);

    vtkSmartPointer<vtkFieldData> fieldData = vtkSmartPointer<vtkFieldData>::New();
    fieldData->AddArray(bboxId);
    fieldData->AddArray(bboxDistances);
    fieldData->AddArray(bboxSizes);
    fieldData->AddArray(bboxCenters);
    fieldData->AddArray(bboxOrientations);
    fieldData->AddArray(bboxLabels);
    bboxId->InsertNextTuple1(static_cast<int>(cluster.ClusterId));
    bboxDistances->InsertNextTuple1(cluster.MeanDepth);
    bboxSizes->InsertNextTuple(cluster.BoundingBox.GetSize().data());
    bboxCenters->InsertNextTuple(cluster.BoundingBox.GetTrueCenter().data());
    bboxOrientations->InsertNextTuple(cluster.BoundingBox.GetOrientation().data());
    bboxLabels->InsertNextTuple1(static_cast<unsigned short>(cluster.ClusterLabel));

    clustersOutput->GetBlock(blockId)->SetFieldData(fieldData);

    ++blockId;
  }

  // Print clusters info
  // Set output for displaying clusters information
  vtkNew<vtkStringArray> data;
  data->SetName("Clusters Information");
  data->SetNumberOfComponents(1);
  std::ostringstream oss;
  for (const auto& cluster : this->ClustersStats)
  {
    oss << std::setprecision(3) << std::showpoint << "Cluster " << std::setw(2) << cluster.ClusterId
        << " : distance = " << std::setw(7) << cluster.MeanDepth << "m  "
        << "size = " << cluster.BoundingBox.GetSize().transpose() << "\n";
  }
  std::string clusterInfo = oss.str();
  data->InsertNextValue(clusterInfo.c_str());
  infoOutput->AddColumn(data);
}

//-----------------------------------------------------------------------------
void vtkMotionDetector::ExtractClustersWithRegionGrowing(vtkSmartPointer<vtkPolyData> input,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput)
{
  if (input->GetNumberOfPoints() == 0)
  {
    vtkLog(INFO, "Not enough motion points: clusters not extracted");
    return;
  }
  // Add array to store cluster Id
  // The array name is ClusterId to be same as the vtkEuclideanClusterExtraction filter
  vtkSmartPointer<vtkIntArray> pointLabel = vtkSmartPointer<vtkIntArray>::New();
  pointLabel->SetName("ClusterId");
  for (auto id = 0; id < input->GetNumberOfPoints(); ++id)
  {
    pointLabel->InsertNextTuple1(-1);
  }
  input->GetPointData()->AddArray(pointLabel);

  // Add point into voxel grid to extract cluster
  Eigen::Vector3d point;
  for (int id = 0; id < input->GetNumberOfPoints(); ++id)
  {
    input->GetPoint(id, point.data());
    // Compute 3D coordinates
    Eigen::Array3i coords =
      ((point - this->ClustersGrid.Origin) / this->ClustersGrid.Resolution).cast<int>();
    if (!this->ClustersGrid.IsInBounds(coords))
      continue;
    // Store point indices of each voxel
    this->ClustersGrid(coords).CurrentPtIndices.emplace_back(id);
  }

  int currentFrame = this->Internals->NbProcessedFrames;
  int unknownLabel = -1;
  // Label time for voxels
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    Voxel& vox = el.second; // shortcut
    if (!vox.CurrentPtIndices.empty())
    {
      vox.ClusterIdx = unknownLabel;
      vox.Time = currentFrame;
    }
  }

  // Link cluster idx to grid indices to speed up search
  std::unordered_map<int, std::vector<int>> clus2gridIndices;
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    int voxelIdx = el.first;  // shortcut
    Voxel& voxel = el.second; // shortcut
    if (voxel.ClusterIdx < 0)
      continue;
    clus2gridIndices[voxel.ClusterIdx].push_back(voxelIdx);
  }
  // Compute centroids of clusters voxel
  std::unordered_map<int, Eigen::Vector3d> clusCentroid;
  clusCentroid.reserve(clus2gridIndices.size());
  for (auto& el : clus2gridIndices)
  {
    auto& clusIdx = el.first;  // shortcut
    auto& indices = el.second; // shortcut
    Eigen::Vector3d centroid(0., 0., 0.);
    for (auto& idx : indices)
    {
      centroid += this->ClustersGrid.To3d(idx).cast<double>().matrix();
    }
    centroid *= this->ClustersGrid.Resolution;
    centroid /= clus2gridIndices[clusIdx].size();
    clusCentroid[clusIdx] = centroid;
  }

  // clang-format off
  const std::vector<Eigen::Array3i> radii = {{ 1,  1, 1}, { 1,  1, 0}, { 1,  1, -1},
                                             { 1,  0, 1}, { 1,  0, 0}, { 1,  0, -1},
                                             { 1, -1, 1}, { 1, -1, 0}, { 1, -1, -1},
                                             { 0,  1, 1}, { 0,  1, 0}, { 0,  1, -1},
                                             { 0,  0, 1},              { 0,  0, -1},
                                             { 0, -1, 1}, { 0, -1, 0}, { 0, -1, -1},
                                             {-1,  1, 1}, {-1,  1, 0}, {-1,  1, -1},
                                             {-1,  0, 1}, {-1,  0, 0}, {-1,  0, -1},
                                             {-1, -1, 1}, {-1, -1, 0}, {-1, -1, -1}};
  // clang-format on
  // Grow existing regions
  for (auto& el : clus2gridIndices)
  {
    auto& clusIdx = el.first;  // shortcut
    auto& indices = el.second; // shortcut

    std::vector<Eigen::Array3i> clusterVoxels;
    clusterVoxels.reserve(indices.size());
    // Fill region with existing pixels
    for (int idx : indices)
      clusterVoxels.push_back(this->ClustersGrid.To3d(idx));

    // Grow region
    int idxVox = 0;
    while (idxVox < int(clusterVoxels.size()))
    {
      // Add neighbors
      for (const Eigen::Array3i& r : radii)
      {
        Eigen::Array3i neigh = clusterVoxels[idxVox] + r;
        if (!this->ClustersGrid.IsInBounds(neigh))
          continue;
        // Check neighbor validity
        if (!this->ClustersGrid.Check(neigh))
          continue;

        // If neighbor is occupied, add it to current cluster
        int neighClusIdx = this->ClustersGrid(neigh).ClusterIdx;

        if (neighClusIdx == unknownLabel)
        {
          // Add neighbor to cluster
          clusterVoxels.push_back(neigh);
          // Update label
          this->ClustersGrid(neigh).ClusterIdx = clusIdx;
          continue;
        }

        // If the neighbor belongs to another existing cluster
        // Check the distance between their centroids
        if (neighClusIdx != clusIdx)
        {
          auto& neighCentroid = clusCentroid[neighClusIdx];
          auto& currCentroid = clusCentroid[clusIdx];
          // Merge clusters if they are close
          if ((neighCentroid - currCentroid).norm() < this->ClusterRadius)
          {
            // Merge the new cluster to the current one
            for (int idx : clus2gridIndices[neighClusIdx])
            {
              // Add to cluster
              clusterVoxels.emplace_back(this->ClustersGrid.To3d(idx));
              // Update label
              this->ClustersGrid.VoxelMap[idx].ClusterIdx = clusIdx;
            }
            // Remove neighbor cluster from clus2gridIndices
            // to not process it afterwards
            clus2gridIndices.erase(neighClusIdx);
          }
        }
      }
      ++idxVox;
    }
  }

  // Fill the potential seeds with remaining unknown pixels
  std::vector<Eigen::Array3i> potentialSeeds;
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    int idx = el.first;     // shortcut
    Voxel& vox = el.second; // shortcut
    if (vox.ClusterIdx == unknownLabel)
      potentialSeeds.push_back(this->ClustersGrid.To3d(idx));
  }
  // Grow new regions
  for (const auto& seed : potentialSeeds)
  {
    // Check seed is still unknown
    if (this->ClustersGrid(seed).ClusterIdx >= 0)
      continue;

    this->ClustersGrid(seed).ClusterIdx = this->NewClusterIdx;

    std::vector<Eigen::Array3i> clusterVoxels;
    clusterVoxels.reserve(this->ClustersGrid.VoxelMap.size());
    clusterVoxels.emplace_back(seed);
    int idxVox = 0;
    while (idxVox < int(clusterVoxels.size()))
    {
      // Check neighbors
      for (const Eigen::Array3i& r : radii)
      {
        Eigen::Array3i neigh = clusterVoxels[idxVox] + r;
        if (!this->ClustersGrid.IsInBounds(neigh))
          continue;
        // If neighbor is occupied, add it to current cluster
        if (this->ClustersGrid.Check(neigh) && this->ClustersGrid(neigh).ClusterIdx == unknownLabel)
        {
          // Add neighbor to cluster
          clusterVoxels.push_back(neigh);
          // Update cluster label of pixel
          this->ClustersGrid(neigh).ClusterIdx = this->NewClusterIdx;
        }
      }
      ++idxVox;
    }
    ++this->NewClusterIdx;
  }

  // Check the validity of clusters
  std::unordered_map<int, int> clusBackground;
  std::unordered_map<int, int> counter;
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    auto& mapId = el.first;
    Voxel& vox = el.second; // shortcut
    if (vox.ClusterIdx < 0)
      continue;
    if (!clusBackground.count(vox.ClusterIdx))
      clusBackground[vox.ClusterIdx] = 0;
    if (this->ClustersGrid.BackgroudMap.count(mapId))
    {
      ++clusBackground[vox.ClusterIdx];
    }
    ++counter[vox.ClusterIdx];
  }
  std::unordered_map<int, bool> clusValidity;
  for (auto& [clusIdx, back] : clusBackground)
  {
    clusValidity[clusIdx] = (float(back) / float(counter[clusIdx]) > 0.5) ? false : true;
  }

  // Remove old and non-valid voxels
  for (auto it = this->ClustersGrid.VoxelMap.begin(); it != this->ClustersGrid.VoxelMap.end();)
  {
    Voxel& vox = it->second; // shortcut
    if (currentFrame - vox.Time > this->TrackingWindowSizes || !clusValidity[vox.ClusterIdx])
    {
      // Erase and return the iterator to the next element
      it = this->ClustersGrid.VoxelMap.erase(it);
      continue;
    }
    // Only increment if not erasing
    ++it;
  }

  // Store point indices for each cluster
  std::unordered_map<int, std::vector<int>> clus2Points;
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    Voxel& voxel = el.second; // shortcut
    if (voxel.ClusterIdx < 0)
    {
      voxel.CurrentPtIndices.clear();
      continue;
    }
    for (const auto& ptId : voxel.CurrentPtIndices)
    {
      clus2Points[voxel.ClusterIdx].emplace_back(ptId);
    }
    // Clear current points of each pixel for next fill
    voxel.CurrentPtIndices.clear();
  }

  // Compute cluster stats: size, mean depth, mean intensity etc
  this->ClustersStats.clear();
  for (const auto& clusPts : clus2Points)
  {
    auto& clusId = clusPts.first;
    auto& clusPtsId = clusPts.second;

    int nbClusterPoints = clusPtsId.size();
    if (nbClusterPoints < this->ClusterMinNbPoints)
      continue;

    // Prepare data for PCA
    vtkNew<vtkPoints> clusterPoints;
    vtkNew<vtkPolyData> clusterPointsPolydata;
    clusterPointsPolydata->SetPoints(clusterPoints);
    vtkNew<vtkDoubleArray> xArray;
    xArray->SetNumberOfComponents(1);
    xArray->SetName("x");
    vtkNew<vtkDoubleArray> yArray;
    yArray->SetNumberOfComponents(1);
    yArray->SetName("y");
    vtkNew<vtkDoubleArray> zArray;
    zArray->SetNumberOfComponents(1);
    zArray->SetName("z");

    ClusterStats clusterInfo;
    // Calculate the average depth value and bounding box for this cluster
    double depth = 0.0;
    double intensity = 0.0;
    for (const auto& pointId : clusPtsId)
    {
      depth += input->GetPointData()->GetArray("distance_m")->GetTuple1(pointId);
      intensity += input->GetPointData()->GetArray("intensity")->GetTuple1(pointId);
      input->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, clusId);
      Eigen::Vector3d point;
      input->GetPoint(pointId, point.data());
      xArray->InsertNextValue(point.x());
      yArray->InsertNextValue(point.y());
      zArray->InsertNextValue(point.z());
      clusterPoints->InsertNextPoint(point.data());
    }

    // For PCA
    vtkNew<vtkTable> datasetTable;
    datasetTable->AddColumn(xArray);
    datasetTable->AddColumn(yArray);
    datasetTable->AddColumn(zArray);
    vtkNew<vtkPCAStatistics> pcaStatistics;
    pcaStatistics->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, datasetTable);
    pcaStatistics->SetColumnStatus("x", 1);
    pcaStatistics->SetColumnStatus("y", 1);
    pcaStatistics->SetColumnStatus("z", 1);
    pcaStatistics->RequestSelectedColumns();
    pcaStatistics->SetDeriveOption(true);
    pcaStatistics->Update();
    vtkNew<vtkDoubleArray> eigenvectors;
    pcaStatistics->GetEigenvectors(eigenvectors);

    Eigen::Vector3d pointcloudCentroid;
    vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter = vtkSmartPointer<vtkCenterOfMass>::New();
    centerOfMassFilter->SetInputData(clusterPointsPolydata);
    centerOfMassFilter->SetUseScalarsAsWeights(false);
    centerOfMassFilter->Update();
    centerOfMassFilter->GetCenter(pointcloudCentroid.data());

    // clang-format off
    double matrix[16] = {
      eigenvectors->GetValue(6), eigenvectors->GetValue(3), eigenvectors->GetValue(0), pointcloudCentroid.x(),
      eigenvectors->GetValue(7), eigenvectors->GetValue(4), eigenvectors->GetValue(1), pointcloudCentroid.y(),
      eigenvectors->GetValue(8), eigenvectors->GetValue(5), eigenvectors->GetValue(2), pointcloudCentroid.z(),
      0., 0., 0., 1. };
    // clang-format on
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(matrix);

    // Transform cluster point cloud
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    transformFilter->SetTransform(transform->GetInverse());
    transformFilter->SetInputData(clusterPointsPolydata);
    transformFilter->Update();
    double bounds[6];
    transformFilter->GetOutput()->GetBounds(bounds);

    clusterInfo.ClusterId = clusId;
    clusterInfo.NbPoints = nbClusterPoints;
    depth /= nbClusterPoints;
    intensity /= nbClusterPoints;
    clusterInfo.MeanDepth = depth;
    clusterInfo.MeanIntensity = intensity;
    clusterInfo.BoundingBox.SetTransform(matrix);
    clusterInfo.BoundingBox.SetVertices(
      bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
    this->ClustersStats.emplace_back(clusterInfo);
  }

  // Label cluster by geometry dimension
  for (auto& cluster : this->ClustersStats)
  {
    if (cluster.BoundingBox.GetSize().z() >= 0.8 && cluster.BoundingBox.GetSize().z() <= 2.2 &&
      ((cluster.BoundingBox.GetSize().x() >= 0.2 && cluster.BoundingBox.GetSize().x() <= 0.7) ||
        (cluster.BoundingBox.GetSize().y() >= 0.2 && cluster.BoundingBox.GetSize().y() <= 0.7)))
    {
      cluster.ClusterLabel = vtkMotionDetector::Label::HUMAN;
    }
    else
    {
      cluster.ClusterLabel = vtkMotionDetector::Label::OTHERS;
    }
  }
  // Sort clusters based on their clusterId
  std::sort(this->ClustersStats.begin(),
    this->ClustersStats.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.ClusterId < cluster2.ClusterId; });

  // Add bounding box for each cluster into output
  int blockId = 0;
  for (const auto& cluster : this->ClustersStats)
  {
    vtkNew<vtkCubeSource> cubeSource;
    vtkNew<vtkPolyData> source;
    cubeSource->SetBounds(cluster.BoundingBox.GetVertices().data());
    cubeSource->SetCenter(cluster.BoundingBox.GetCenter().data());
    // Transform bounding box
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(cluster.BoundingBox.GetTransform().data());
    transformFilter->SetTransform(transform);
    transformFilter->SetInputData(source);
    transformFilter->SetInputConnection(cubeSource->GetOutputPort());
    transformFilter->Update();
    source->ShallowCopy(transformFilter->GetOutput());

    std::string blockName("Cluster-" + std::to_string(cluster.ClusterId));
    clustersOutput->SetBlock(blockId, source);
    clustersOutput->GetMetaData(blockId)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());

    // Create field data and add information to it
    vtkSmartPointer<vtkIntArray> bboxId = vtkSmartPointer<vtkIntArray>::New();
    bboxId->SetName("ClusterId");
    bboxId->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxDistances = vtkSmartPointer<vtkFloatArray>::New();
    bboxDistances->SetName("Distance");
    bboxDistances->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxSizes = vtkSmartPointer<vtkFloatArray>::New();
    bboxSizes->SetName("Size");
    bboxSizes->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxCenters = vtkSmartPointer<vtkFloatArray>::New();
    bboxCenters->SetName("Center");
    bboxCenters->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxOrientations = vtkSmartPointer<vtkFloatArray>::New();
    bboxOrientations->SetName("Orientation");
    bboxOrientations->SetNumberOfComponents(4);
    vtkSmartPointer<vtkUnsignedShortArray> bboxLabels =
      vtkSmartPointer<vtkUnsignedShortArray>::New();
    bboxLabels->SetName("Label");
    bboxLabels->SetNumberOfComponents(1);

    vtkSmartPointer<vtkFieldData> fieldData = vtkSmartPointer<vtkFieldData>::New();
    fieldData->AddArray(bboxId);
    fieldData->AddArray(bboxDistances);
    fieldData->AddArray(bboxSizes);
    fieldData->AddArray(bboxCenters);
    fieldData->AddArray(bboxOrientations);
    fieldData->AddArray(bboxLabels);
    bboxId->InsertNextTuple1(static_cast<int>(cluster.ClusterId));
    bboxDistances->InsertNextTuple1(cluster.MeanDepth);
    bboxSizes->InsertNextTuple(cluster.BoundingBox.GetSize().data());
    bboxCenters->InsertNextTuple(cluster.BoundingBox.GetTrueCenter().data());
    bboxOrientations->InsertNextTuple(cluster.BoundingBox.GetOrientation().data());
    bboxLabels->InsertNextTuple1(static_cast<unsigned short>(cluster.ClusterLabel));

    clustersOutput->GetBlock(blockId)->SetFieldData(fieldData);

    ++blockId;
  }

  // Print clusters info
  // Set output for displaying clusters information
  vtkNew<vtkStringArray> data;
  data->SetName("Clusters Information");
  data->SetNumberOfComponents(1);
  std::ostringstream oss;
  for (const auto& cluster : this->ClustersStats)
  {
    oss << std::setprecision(3) << std::showpoint << "Cluster " << std::setw(2) << cluster.ClusterId
        << " : distance = " << std::setw(7) << cluster.MeanDepth << "m  "
        << "size = " << cluster.BoundingBox.GetSize().transpose() << "\n";
  }
  std::string clusterInfo = oss.str();
  data->InsertNextValue(clusterInfo.c_str());
  infoOutput->AddColumn(data);
}

//-----------------------------------------------------------------------------
int vtkMotionDetector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkLog(INFO, "Processing frame #" << this->Internals->NbProcessedFrames);

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
  if (!input || input->GetNumberOfPoints() < 1)
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
  vtkTable* clusterInfoOutput = vtkTable::GetData(outputVector, CLUSTERS_TEXT_OUTPUT_PORT);

  // Compute azimuth and vertical angles bounds
  if (this->Internals->NbProcessedFrames == 0)
    this->Internals->ComputeBounds(input);

  if (this->ClusterExtractor == static_cast<int>(vtkMotionDetector::Extractor::REGION_GROWING))
    this->InitClusteringGrid(input);

  if (this->Internals->NbProcessedFrames < this->InitNbFrames)
    vtkLog(INFO, "Waiting for the initialization");

  // Estimate probability of a point and update GMM
  vtkSmartPointer<vtkPolyData> motionPolydata = vtkSmartPointer<vtkPolyData>::New();
  this->EstimateMotion(input, motionPolydata);

  // Extract clusters on the motion points
  if (this->Internals->NbProcessedFrames >= this->InitNbFrames &&
    this->ClusterExtractor != static_cast<int>(vtkMotionDetector::Extractor::NOEXTRACTION))
  {
    switch (this->ClusterExtractor)
    {
      case static_cast<int>(vtkMotionDetector::Extractor::EUCLIDEAN):
      {
        this->ExtractClustersWithEuclidean(motionPolydata, clustersOutput, clusterInfoOutput);
        break;
      }
      case static_cast<int>(vtkMotionDetector::Extractor::GMM):
      {
        this->ExtractClustersWithGMM(motionPolydata, clustersOutput, clusterInfoOutput);
        break;
      }
      case static_cast<int>(vtkMotionDetector::Extractor::REGION_GROWING):
      {
        this->ExtractClustersWithRegionGrowing(motionPolydata, clustersOutput, clusterInfoOutput);
        break;
      }
    }
  }

  motionPointsOutput->ShallowCopy(motionPolydata);

  ++this->Internals->NbProcessedFrames;

  return 1;
}

//-----------------------------------------------------------------------------
void vtkMotionDetector::InitClusteringGrid(vtkPolyData* polydata)
{
  if (this->Internals->NbProcessedFrames > this->InitNbFrames)
    return;
  // Initialize parameters of clustering grid
  if (this->Internals->NbProcessedFrames == 0)
  {
    double bounds[6];
    polydata->GetBounds(bounds);
    this->ClustersGrid.Origin = { bounds[0], bounds[2], bounds[4] };
    this->ClustersGrid.Resolution = this->ClusterGridResolution;
    for (int i = 0; i < 3; i++)
    {
      this->ClustersGrid.GridSize[i] =
        std::ceil((bounds[2 * i + 1] - bounds[2 * i]) / this->ClustersGrid.Resolution);
    }
  }
  // Update background grid
  Eigen::Vector3d point;
  for (vtkIdType id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    // Count number of points for each voxel
    polydata->GetPoint(id, point.data());
    Eigen::Array3i coords =
      ((point - this->ClustersGrid.Origin) / this->ClustersGrid.Resolution).cast<int>();
    if (this->ClustersGrid.IsInBounds(coords))
      ++this->ClustersGrid.BackgroudMap[this->ClustersGrid.To1d(coords)].NbCurrentPts;
  }
  for (auto& el : this->ClustersGrid.BackgroudMap)
  {
    auto& vox = el.second;
    if (vox.NbCurrentPts > 0)
      ++vox.SeenTimes;
    vox.NbCurrentPts = 0;
  }
  // Remove voxels which are not considered as part of the background
  // at the end of initialization phase
  if (this->Internals->NbProcessedFrames == this->InitNbFrames)
  {
    for (auto it = this->ClustersGrid.BackgroudMap.begin();
         it != this->ClustersGrid.BackgroudMap.end();)
    {
      Voxel& vox = it->second; // shortcut
      // A voxel is not background if it has been seen less than 5% of initial time
      if (vox.SeenTimes < 0.05 * this->InitNbFrames)
      {
        it = this->ClustersGrid.BackgroudMap.erase(it);
        continue;
      }
      ++it;
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkMotionDetector::IdentifyInputArrays(vtkPolyData* polydata)
{
  // Get lidar vendor if first frame
  bool valid = true;
  if (this->Internals->NbProcessedFrames == 0)
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
    vtkLog(INFO, "Lidar vendor is identified as " << vendorName);
  }

  // Check if the requested arrays exist or not
  switch (this->Internals->Lidar)
  {
    case vtkInternals::LidarVendor::VELODYNE:
    {
      valid = polydata->GetPointData()->HasArray("azimuth") &&
        polydata->GetPointData()->HasArray("vertical_angle") &&
        polydata->GetPointData()->HasArray("distance_m") &&
        polydata->GetPointData()->HasArray("intensity");
      break;
    }
    case vtkInternals::LidarVendor::LIVOX:
    case vtkInternals::LidarVendor::HESAI:
    {
      valid = polydata->GetPointData()->HasArray("distance_m") &&
        polydata->GetPointData()->HasArray("intensity");
      break;
    }
    default:
    {
      break;
    }
  }
  return valid;
}
