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
  vtkSetMacro(ClusterRadius, double);
  // Set the minimum number of points in a cluster
  vtkSetMacro(ClusterMinNbPoints, int);
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

  // Extract clusters with vtkEuclideanClusterExtraction method
  void ExtractClustersWithEuclidean(vtkSmartPointer<vtkPolyData> polydata,
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
