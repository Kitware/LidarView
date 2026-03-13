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

#define _USE_MATH_DEFINES
#include <cmath>

// VTK includes
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPointsPCA.h>
#include <vtkSMPTools.h>
#include <vtkVector.h>

#include "NormalsEstimator.h"

//----------------------------------------------------------------------------
void NormalEstimator::SetNoise(float noise)
{
  this->Noise = std::max(noise, 1e-6f);
  this->ThreshOutliers = std::pow(this->Noise / 3, 2);
}

//----------------------------------------------------------------------------
void NormalEstimator::ComputeNeighDistances(const Eigen::Vector3f& point)
{
  this->Distances.resize(this->NeighborsIndices.size(), 3);
  for (int idx = 0; idx < static_cast<int>(this->NeighborsIndices.size()); idx++)
  {
    double* neighbour = this->CloudManager->at(idx);
    Eigen::Vector3f neighbourEigen(static_cast<float>(neighbour[0]),
      static_cast<float>(neighbour[1]),
      static_cast<float>(neighbour[2]));

    this->Distances.row(idx) = neighbourEigen - point;
  }
}

//----------------------------------------------------------------------------
Eigen::Vector3f NormalEstimator::GetLowestEigenVector(const Eigen::Matrix3f& covarianceMatrix) const
{
  Eigen::Matrix<float, 3, 3> eigenVectors;
  Eigen::Matrix<float, 3, 1> eigenValues;

  // Perform eigen decomposition
  Eigen::EigenSolver<Eigen::Matrix3f> solver(covarianceMatrix);
  eigenValues = solver.eigenvalues().real();
  eigenVectors = solver.eigenvectors().real();

  int minIndex;
  eigenValues.minCoeff(&minIndex);
  return eigenVectors.col(minIndex);
}

//----------------------------------------------------------------------------
float NormalEstimator::Init(Normal& normal)
{
  // Compute PCA
  Eigen::Matrix<float, 3, 3> eigenVectors;
  Eigen::Matrix<float, 3, 1> eigenValues;
  vtkSmartPointer<vtkPointsPCA> pcaFilter = vtkSmartPointer<vtkPointsPCA>::New();

  pcaFilter->SetInputData(this->CloudManager->GetCloud());
  pcaFilter->SetPointIndices(this->NeighborsIndices);
  pcaFilter->Update();

  // Get eigen vectors
  vtkSmartPointer<vtkDoubleArray> eigenVectorsVTK = vtkSmartPointer<vtkDoubleArray>::New();
  pcaFilter->GetEigenVectors(eigenVectorsVTK);

  // Get eigen values
  vtkVector3d eigenValuesVTK = pcaFilter->GetEigenValues();
  eigenValues[0] = eigenValuesVTK.GetX();
  eigenValues[1] = eigenValuesVTK.GetY();
  eigenValues[2] = eigenValuesVTK.GetZ();

  for (vtkIdType row = 0; row < 3; ++row)
  {
    for (vtkIdType col = 0; col < 3; ++col)
    {
      eigenVectors(row, col) = static_cast<float>(eigenVectorsVTK->GetComponent(col, row));
    }
  }
  eigenVectors.col(0).swap(eigenVectors.col(2));
  eigenValues.row(0).swap(eigenValues.row(2));

  // Get normal as the eigen vector corresponding to the lowest eigen value
  normal.Dir = eigenVectors.cast<float>().col(0);

  float meanError = (this->Distances * normal.Dir).sum();
  if (meanError < 0.f)
  {
    normal.Dir *= -1.f;
  }

  return std::sqrt(eigenValues(0));
}

//----------------------------------------------------------------------------
void NormalEstimator::ComputeWeights(const Eigen::Vector3f& normalDir, float threshOutliers)
{
  this->Weights.resize(this->NeighborsIndices.size());
  for (int i = 0; i < static_cast<int>(this->NeighborsIndices.size()); i++)
  {
    this->Weights(i) = std::pow(
      threshOutliers / (threshOutliers + std::pow(normalDir.dot(this->Distances.row(i)), 2)), 2);
  }
}

//----------------------------------------------------------------------------
float NormalEstimator::ComputeMaxError(const Normal& normal) const
{
  return (this->Distances * normal.Dir).array().abs().maxCoeff();
}

//----------------------------------------------------------------------------
void NormalEstimator::Optimize(Normal& normal, float muStart, bool movePt)
{
  float mu = muStart;
  unsigned int nbItr = 0;
  while (mu > this->ThreshOutliers || nbItr < this->ItrMin)
  {
    if (mu > this->ThreshOutliers)
    {
      mu /= this->DivFact;
    }

    this->ComputeWeights(normal.Dir, mu);

    if (movePt)
    {
      // Update position of the PCA reference
      // Threshold the weights
      Eigen::VectorXf weightsThresholded;
      weightsThresholded.array() =
        this->Weights.array() * (this->Weights.array() > this->ThreshWeight).cast<float>();
      float sumWeights = weightsThresholded.sum();
      float motion =
        ((weightsThresholded.array() * (this->Distances * normal.Dir).array()).matrix()).sum();

      // If no neighbor is trustworthy enough, stop refinement and keep initial normal
      // TODO: find a better method to deal with sumWeights when it equals 0
      if (sumWeights == 0)
      {
        break;
      }

      motion /= sumWeights;
      normal.Point += motion * normal.Dir;
      // Recompute distances to new point
      this->ComputeNeighDistances(normal.Point);
      this->ComputeWeights(normal.Dir, mu);
    }

    // Update Normal
    Eigen::Matrix3f covarianceMatrix =
      this->Distances.transpose() * this->Weights.asDiagonal() * this->Distances;
    normal.Dir = this->GetLowestEigenVector(covarianceMatrix);
    ++nbItr;
  }

  float meanError = (this->Distances * normal.Dir).sum();
  if (meanError < 0.f)
  {
    normal.Dir *= -1.f;
  }
}

//----------------------------------------------------------------------------
void NormalEstimator::Orient(Normal& normal) const
{
  if (normal.Dir.dot(normal.Point - this->OrientPoint) < 0)
  {
    normal.Dir *= -1.f;
  }
}

//----------------------------------------------------------------------------
int NormalEstimator::CountInliers() const
{
  int nbInliers = 0;
  for (int i = 0; i < static_cast<int>(this->NeighborsIndices.size()); ++i)
  {
    if (this->Weights(i) > this->ThreshWeight)
    {
      ++nbInliers;
    }
  }
  return nbInliers;
}

//----------------------------------------------------------------------------
Normal NormalEstimator::SelectBestNormal(const Normal& first,
  int nbInliersFirst,
  const Normal& second,
  int nbInliersSecond,
  Eigen::Vector3f& refPoint) const
{
  int minPointsNb = this->MinInliersRatio * this->NeighborsIndices.size();
  if (nbInliersFirst > minPointsNb && nbInliersSecond < minPointsNb)
  {
    return first;
  }
  if (nbInliersFirst < minPointsNb && nbInliersSecond > minPointsNb)
  {
    return second;
  }

  float ptMotionFirst = std::abs((first.Point - refPoint).dot(first.Dir));
  float ptMotionSecond = std::abs((second.Point - refPoint).dot(second.Dir));

  if (ptMotionFirst < ptMotionSecond)
  {
    return first;
  }

  return second;
}

//----------------------------------------------------------------------------
Normal NormalEstimator::EstimateNormal(int idxPt, const std::vector<int>& indices)
{
  Normal normal;
  double* pt = this->CloudManager->at(idxPt);
  normal.Point[0] = static_cast<float>(pt[0]);
  normal.Point[1] = static_cast<float>(pt[1]);
  normal.Point[2] = static_cast<float>(pt[2]);

  // Extract neighborhood
  if (indices.empty())
  {
    this->CloudManager->SearchNeighbors(idxPt, this->NeighborsIndices);
  }
  else
  {
    this->NeighborsIndices = indices;
  }

  // Compute first initialization
  float rmse = this->Init(normal);

  // Check if the point needs optimization
  if (rmse > this->Noise)
  {
    Normal initNormal = normal;
    this->ComputeNeighDistances(normal.Point);
    // Compute the maximum error of a neighbor to the estimated plane
    float errMax = this->ComputeMaxError(initNormal);

    // Compute first solution
    // Optimize only normal
    // Set starting threshold to half the maximum error
    float muStart = std::max((float)std::pow(errMax, 2), this->ThreshOutliers);
    this->Optimize(normal, muStart);
    // Optimize normal and point
    this->Optimize(normal, muStart, true);
    Normal firstRefined = normal;
    // Count number of inliers relatively to current weights
    int nbInliersFirst = this->CountInliers();

    // If the normal is different between initialization and refine steps
    // this might mean there are various surface pieces in the neighborhood
    if ((std::abs(initNormal.Dir.dot(firstRefined.Dir)) < std::cos(this->MaxAngle)))
    {
      // Compute second solution
      // Reinit point
      normal.Point = initNormal.Point;
      // Init normal to the perpendicular to edge and first normal
      Eigen::Vector3f edgeDirection = initNormal.Dir.cross(firstRefined.Dir).normalized();
      normal.Dir = (edgeDirection.cross(firstRefined.Dir)).normalized();
      // Optimize only normal
      // Set starting threshold to the tercil error of the neighbors
      Eigen::VectorXf projectionErrorSquared = (this->Distances * normal.Dir).array().pow(2);
      std::sort(projectionErrorSquared.data(),
        projectionErrorSquared.data() + projectionErrorSquared.size());
      int tercilIdx = this->NeighborsIndices.size() / 3;
      muStart = std::max(projectionErrorSquared[tercilIdx], this->ThreshOutliers);

      this->Optimize(normal, muStart);
      // Set starting threshold to half of the first starting threshold
      muStart = std::max(0.1f * muStart, this->ThreshOutliers);
      // Optimize normal and point
      this->Optimize(normal, muStart, true);
      Normal secondRefined = normal;
      // Count number of inliers relatively to current weights
      int nbInliersSecond = this->CountInliers();

      // Evaluate
      normal = this->SelectBestNormal(
        firstRefined, nbInliersFirst, secondRefined, nbInliersSecond, initNormal.Point);
    }
  }

  if (this->ApplyOrientation)
  {
    this->Orient(normal);
  }

  return normal;
}

//----------------------------------------------------------------------------
// Main API functions
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
  bool denoise)
{
  // Init output
  cloudWithNormals->DeepCopy(cloud);
  // Create searcher
  std::shared_ptr<NeighborsSearch> cloudManager(new NeighborsSearch(cloud, Search::Mode::KNN));
  // Set required number of neighbors
  cloudManager->SetKnn(knn);
  // Set min radius acceptable
  cloudManager->SetRadius(radius);
  // Create vtk normals
  vtkSmartPointer<vtkFloatArray> normals = vtkSmartPointer<vtkFloatArray>::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(cloud->GetNumberOfPoints());
  normals->SetName("Normals");

  vtkSMPTools::For(0,
    cloudManager->size(),
    [&](vtkIdType start, vtkIdType end)
    {
      for (int ptIdx = start; ptIdx < end; ++ptIdx)
      {
        NormalEstimator estimator(cloudManager, noise, 5.f * M_PI / 180.f);
        estimator.SetApplyOrientation(orient);
        Normal normal = estimator.EstimateNormal(ptIdx);
        if (denoise)
          cloudWithNormals->GetPoints()->SetPoint(
            ptIdx, normal.Point[0], normal.Point[1], normal.Point[2]);
        normals->InsertTuple3(ptIdx, normal.Dir[0], normal.Dir[1], normal.Dir[2]);
      }
    });
  cloudWithNormals->GetPointData()->SetNormals(normals);
}

//----------------------------------------------------------------------------
void ComputeNormalsRadius(vtkPolyData* cloud,
  vtkPolyData* cloudWithNormals,
  float radius,
  float noise,
  int knn,
  bool orient,
  bool denoise)
{
  // Init output
  cloudWithNormals->DeepCopy(cloud);
  // Create searcher
  std::shared_ptr<NeighborsSearch> cloudManager(new NeighborsSearch(cloud, Search::Mode::RADIUS));
  // Set radius search
  cloudManager->SetRadius(radius);
  // Set min acceptable number of neighbors
  cloudManager->SetKnn(knn);
  // Create vtk normals
  vtkSmartPointer<vtkFloatArray> normals = vtkSmartPointer<vtkFloatArray>::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(cloud->GetNumberOfPoints());
  normals->SetName("Normals");

  vtkSMPTools::For(0,
    cloudManager->size(),
    [&](vtkIdType start, vtkIdType end)
    {
      for (int ptIdx = start; ptIdx < end; ++ptIdx)
      {
        NormalEstimator estimator(cloudManager, noise, 5.f * M_PI / 180.f);
        estimator.SetApplyOrientation(orient);
        Normal normal = estimator.EstimateNormal(ptIdx);
        if (denoise)
          cloudWithNormals->GetPoints()->SetPoint(
            ptIdx, normal.Point[0], normal.Point[1], normal.Point[2]);
        normals->InsertTuple3(ptIdx, normal.Dir[0], normal.Dir[1], normal.Dir[2]);
      }
    });
  cloudWithNormals->GetPointData()->SetNormals(normals);
}
} // End of NormalEstimation namespace