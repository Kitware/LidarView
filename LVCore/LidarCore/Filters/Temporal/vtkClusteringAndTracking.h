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

#ifndef VTK_CLUSTERING_AND_TRACKING_H
#define VTK_CLUSTERING_AND_TRACKING_H

// VTK
#include <vtkMultiBlockDataSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkTableAlgorithm.h>

// EIGEN
#include <Eigen/Dense>

#include "lvFiltersTemporalModule.h"

#include <climits>
#include <list>

/**
 * @brief The ClusteringAndTracking class contains three algorithms to
 * extract clusters on the input pointcloud, which two of them can give a
 * rough tracking of clusters.
 * Input: Pointcloud
 * Output1: Clusters points
 * Output2: Clusters statistics
 * Output3: Text of clusters information for display purpose
 */
class LVFILTERSTEMPORAL_EXPORT vtkClusteringAndTracking : public vtkPolyDataAlgorithm
{
public:
  static vtkClusteringAndTracking* New();
  vtkTypeMacro(vtkClusteringAndTracking, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Getters / Setters
  // Set the extraction method
  void SetClusterExtractor(int extractor);
  // Set the minimum radius of a cluster
  void SetClusterRadius(double radius);
  // Set the minimum number of points in a cluster
  void SetClusterMinNbPoints(int minNbPoints);

  // Set tracking window sizes
  void SetTrackingWindowSizes(int trackingWindowSizes);
  // Setter to enable/disable to compute the cluster's orientation
  vtkSetMacro(EnableClusterOrientation, bool);
  vtkGetMacro(EnableClusterOrientation, bool);

protected:
  // constructor / destructor
  vtkClusteringAndTracking();
  ~vtkClusteringAndTracking();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  // copy operators
  vtkClusteringAndTracking(const vtkClusteringAndTracking&);
  void operator=(const vtkClusteringAndTracking&);

  // Parameters to extract clusters
  enum Extractor
  {
    NOEXTRACTION = 0,
    EUCLIDEAN = 1,
    GMM = 2,
    REGION_GROWING = 3,
  };
  Extractor ClusterExtractor = Extractor::NOEXTRACTION;
  // Minimum radius of a cluster
  double ClusterRadius = 0.4;
  // Minimum number of points of a cluster
  int ClusterMinNbPoints = 5;

  // Parameter to enable/disable to compute orientation of clusters
  bool EnableClusterOrientation = false;

  // Parameters for tracking
  int TrackingWindowSizes = 10;

  // Bounding box of clusters
  class Bbox
  {
  public:
    void SetVertices(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
    {
      this->Vertices = { xmin, xmax, ymin, ymax, zmin, zmax };
      this->Center = { (xmax + xmin) / 2., (ymax + ymin) / 2, (zmax + zmin) / 2 };
      this->Size = { xmax - xmin, ymax - ymin, zmax - zmin };
    };
    void SetTransform(double trans[16])
    {
      this->Transform.resize(16);
      for (int i = 0; i < 16; i++)
        this->Transform[i] = trans[i];
    };
    Eigen::Vector3d GetTrueCenter() const;
    Eigen::Vector4d GetOrientation() const;

    Eigen::Matrix<double, 6, 1> GetVertices() const { return this->Vertices; }
    Eigen::Vector3d GetSize() const { return this->Size; }
    Eigen::Vector3d GetCenter() const { return this->Center; }
    std::vector<double> GetTransform() const { return this->Transform; }

  private:
    Eigen::Isometry3d GetEigenTransform() const;
    std::vector<double>
      Transform = { 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1. };
    Eigen::Matrix<double, 6, 1> Vertices = { 0., 0., 0., 0., 0., 0. };
    Eigen::Vector3d Center = { 0., 0., 0. };
    Eigen::Vector3d Size = { 0., 0., 0. };
  };

  // Labels of clusters
  enum Label
  {
    HUMAN = 0,
    OTHERS = 1,
  };

  // The statistic of clusters
  struct ClusterStats
  {
    ClusterStats() = default;
    Label ClusterLabel = Label::HUMAN;
    int ClusterId = 0;
    int NbPoints = 0;
    double MeanDepth = 0.;
    double MeanIntensity = 0.;
    Bbox BoundingBox;
  };
  std::vector<ClusterStats> ClustersStats;

  // Clustering with gaussian mixture model
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
    // Indices of points added into gaussian
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
    // Update the time to live
    // Return false if TTL falls to zero value
    bool UpdateTTL()
    {
      this->TTL -= 1;
      return (this->TTL >= 0);
    }
    // Update parameters using new value and the weight of this value
    void UpdateParams(const Eigen::Vector3d& pt, double weightX);
  };

  /**
   * @brief The GaussianMixture class in vtkClusteringAndTracking class constructs a gaussian
   * mixture model to extract cluster of input points and track clusters frame by frame.
   * Each gaussian distribution represents a cluster with a weight and a cluster id label
   */
  class GaussianMixture3D
  {
  public:
    // Default constructor
    GaussianMixture3D() = default;

    // Clear all gaussians distribution in GMM and reset cluster ID counter
    void Reset();

    // Getters / Setters
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

    // Set value to initialize covariance with the required cluster size
    void SetCovCoeff(double clusterSize) { this->CovCoeff = std::pow(clusterSize, 2.); }

    void SetGaussianClusterRadius(double clusterSize) { this->GaussianClusterRadius = clusterSize; }
    void SetGaussianClusterMinNbPoints(int minNumber)
    {
      this->GaussianClusterMinNbPoints = minNumber;
    }

    // Increment Unique ID Counter
    int GetNewUniqueID()
    {
      ++this->UniqueID;
      return this->UniqueID - 1;
    }

    // Reset time to live value to its maximum when a new point is added to a gaussian distribution
    void ResetTTL() { this->ItClosest->TTL = this->MaxTTL; }

    /**
     * @brief Add the new point into gaussian mixture model
     * If the model is empty, add a new cluster with weight = 1
     * If the point is too far from existing clusters, add a new cluster with appropriate weight
     * Otherwise, add the point into model and update parameters
     */
    void AddPoint(const Eigen::Vector3d& pt, int ptId);

    // Getter to obtain points indices and cluster id label of extracted clusters for a frame
    void GetClusters(std::vector<std::vector<int>>& clusters, std::vector<int>& clusterId);

    // Reset cluster PointsId to store new clusters for next frame
    void ClearClusters();

    /**
     * @brief Update the time to live of the each gaussian in the model
     * If a gaussian is dead, erase it. Then the weight of the remains
     * gaussians need to be normalized and the motion labels need to be updated
     */
    void UpdateClusters();

  private:
    // counter for a new cluster ID
    int UniqueID = 0;
    // Maximum number of frames for TTL
    int MaxTTL = 10;
    // To initialiaze covariance with 0.2 research radius
    double CovCoeff = 0.04;
    // Minimum points to form a cluster
    int GaussianClusterMinNbPoints = 10;
    // Radius to search for cluster
    double GaussianClusterRadius = 0.5;
    // Iterator to the cluster whose mean value is the closest to the point
    std::list<Gaussian3D>::iterator ItClosest;
    // Gaussian distributions
    std::list<Gaussian3D> Gaussians;
  };
  GaussianMixture3D GMMClusters;

  // Extract clusters with vtkEuclideanClusterExtraction method
  void ExtractClustersWithEuclidean(vtkSmartPointer<vtkPolyData> polydata,
    vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
    vtkSmartPointer<vtkTable> infoOutput);

  // Extract clusters with gaussian mixture model method
  void ExtractClustersWithGMM(vtkSmartPointer<vtkPolyData> polydata,
    vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
    vtkSmartPointer<vtkTable> infoOutput);

  // Compute stats of a cluster
  ClusterStats ComputeClusterStats(vtkSmartPointer<vtkPolyData> input,
    const std::vector<int>& clusterPtIndices,
    const int clusterId);

  // Create outputs
  void CreateClustersOutput(vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
    vtkSmartPointer<vtkTable> infoOutput);
};

#endif // VTK_CLUSTERING_H
