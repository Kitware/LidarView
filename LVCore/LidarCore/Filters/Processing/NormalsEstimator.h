/*=========================================================================

  Program: LidarView
  Module:  vtkComputeNormalAdaptive.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef NORMAL_ESTIMATOR
#define NORMAL_ESTIMATOR

#include <algorithm>
#include <chrono>
#include <float.h>
#include <math.h>
#include <utility>
#include <vector>

#include "KDTreeVTKAdaptor.h"
#include "NeighborsSearch.h"

struct Normal
{
  Eigen::Vector3f Dir;
  Eigen::Vector3f Point;
};

/**
 * NormalEstimator computes normal vector of a point using iterative weighted PCA
 * based on the algorithm described in
 * https://perso.liris.cnrs.fr/david.coeurjolly/publication/sanchez-20/sanchez-20.pdf
 */
class NormalEstimator
{
public:
  //------------------------------------------------------------------------------
  // Public API
  //------------------------------------------------------------------------------

  NormalEstimator() = default;

  NormalEstimator(std::shared_ptr<NeighborsSearch> cloudManager, float noise, float maxAngle)
    : CloudManager(cloudManager)
    , MaxAngle(maxAngle)
  {
    this->SetNoise(noise);
  };

  // Parameters setters/getters
  float GetDivFact() const { return this->DivFact; }
  void SetDivFact(float divFact) { this->DivFact = divFact; }

  float GetNoise() const { return this->Noise; }
  void SetNoise(float noise);

  float GetMaxAngle() const { return this->MaxAngle; }
  void SetMaxAngle(float maxAngle) { this->MaxAngle = maxAngle; }

  unsigned int GetItrMin() const { return this->ItrMin; }
  void SetItrMin(unsigned int itrMin) { this->ItrMin = itrMin; }

  bool GetApplyOrientation() const { return this->ApplyOrientation; }
  void SetApplyOrientation(bool orient) { this->ApplyOrientation = orient; }

  float GetMinInliersRatio() const { return this->MinInliersRatio; }
  void SetMinInliersRatio(float minInliersRatio) { this->MinInliersRatio = minInliersRatio; }

  Eigen::Vector3f GetOrientPoint() const { return this->OrientPoint; }
  void SetOrientPoint(const Eigen::Vector3f& orientPoint) { this->OrientPoint = orientPoint; }

  // Input setters/getters
  std::shared_ptr<NeighborsSearch> GetCloudManager() const { return this->CloudManager; }
  void SetCloudManager(std::shared_ptr<NeighborsSearch> cloudManager)
  {
    this->CloudManager = cloudManager;
  }

  Normal EstimateNormal(int idxPt, const std::vector<int>& indices = {});

private:
  //------------------------------------------------------------------------------
  // Internal methods
  //------------------------------------------------------------------------------

  // Tools------------------------------------------------------------------------

  // Fill Dist with distances of the neighbors from current PCA reference point
  void ComputeNeighDistances(const Eigen::Vector3f& point);

  // Compute the weights associated to the neighbors
  // This uses a scaled version of Geman McClure estimator
  void ComputeWeights(const Eigen::Vector3f& normal, float threshOutliers);

  // Compute maximal residual
  float ComputeMaxError(const Normal& normal) const;

  // Count neighbor inliers relatively to the weight threshold
  int CountInliers() const;

  // Compute the eigen vector corresponding
  // to the lowest eigen value of a covariance matrix
  Eigen::Vector3f GetLowestEigenVector(const Eigen::Matrix3f& covarianceMatrix) const;

  // Pipeline---------------------------------------------------------------------

  // Calculate first initialization with PCA + initialize mu_lim and tau
  float Init(Normal& normal);

  // Rough estimation
  void Optimize(Normal& normal, float muStart, bool movePt = false);

  // Select appropriate normal
  Normal SelectBestNormal(const Normal& first,
    int nbInliersFirst,
    const Normal& second,
    int nbInliersSecond,
    Eigen::Vector3f& refPoint) const;

  // Orient normal to the exterior of the edge
  void Orient(Normal& normal) const;

  //------------------------------------------------------------------------------
  // Members
  //------------------------------------------------------------------------------

  // Input------------------------------------------------------------------------

  std::shared_ptr<NeighborsSearch> CloudManager;

  // Parameters

  // Noise in the processed point cloud
  // This can be evaluated as the "width" of a planar surface
  float Noise = 0.01f;

  // Minimal outlier thresholds
  float ThreshOutliers = 1e-6f;

  // Minimum ratio of inliers in the neighbors
  // to consider the output normal
  float MinInliersRatio = 0.25f;

  // Maximal angle (in radians) to consider that two unit vectors are similar
  float MaxAngle = 0.087f;

  // Division factor to modify the outlier threshold through iterations
  float DivFact = 1.01f;

  // Minimum number of iterations to perform
  unsigned int ItrMin = 5;

  // Weight threshold to define an inlier
  // Weights are contained between 0 and 1
  float ThreshWeight = 0.25f;

  // Compute orientation if true
  bool ApplyOrientation = true;

  // Point towards which to orient the normals if required
  Eigen::Vector3f OrientPoint = { 0.0, 0.0, 0.0 };

  // Intermediate results for storage

  // Indices of the neighbors of an input point
  std::vector<int> NeighborsIndices;
  // Distances between the neighbors and an input point
  Eigen::Matrix<float, Eigen::Dynamic, 3> Distances;
  // Weights attached to the neighbors during optimization
  Eigen::VectorXf Weights;
};

//----------------------------------------------------------------------------
// Main API functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
namespace NormalEstimation
{
//----------------------------------------------------------------------------
void ComputeNormalsKnn(vtkPolyData* cloud,
  vtkPolyData* cloudWithNormals,
  int knn,
  float noise,
  float radius,
  bool orient,
  bool denoise);

//----------------------------------------------------------------------------
void ComputeNormalsRadius(vtkPolyData* cloud,
  vtkPolyData* cloudWithNormals,
  float radius,
  float noise,
  int knn,
  bool orient,
  bool denoise);
} // End of NormalEstimation namespace

#endif // NORMAL_ESTIMATOR