// Copyright 2018 Kitware SAS
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
  Module:    vtkMotionDetector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_MOTION_DETECTOR_H
#define VTK_MOTION_DETECTOR_H

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
#include <memory> // for std::unique_ptr

/**
 * @brief The MotionDetector class constructs a spherical map of depth along the
 * vertical angle and the azimuth angle. A gaussian mixture model is built
 * at each pixel and it is updated when a new point arrived. The probability
 * that a point is belong to background can be evaluated by GMM.
 * Input: Lidar frame
 * Output1: Detected motion points
 * Output2: Clusters statistics
 * Output3: Text of clusters information for display purpose
 */
class LVFILTERSTEMPORAL_EXPORT vtkMotionDetector : public vtkPolyDataAlgorithm
{
public:
  static vtkMotionDetector* New();
  vtkTypeMacro(vtkMotionDetector, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Reset detection
  void Reset();

  // Set resolution of spherical map
  void SetVerticalResolution(double verticalReso);
  void SetAzimuthResolution(double azimuthReso);

  // Set duration of gaussian model
  void SetWindowSize(int windowSize);

  // Set Max distance to apply motion detection
  void SetDetectionRange(double minDist, double maxDist);

  // Set number of frames to wait for initialization
  vtkSetMacro(InitNbFrames, int);

  // Set radius and neighbor value to remove isolated motion points
  vtkSetMacro(SubsampleRange, double);
  vtkSetMacro(SubsampleResolution, double);
  vtkSetMacro(RemovalOutlierRadius, double);
  vtkSetMacro(RemovalOutlierNeighbors, int);

  // Set parameters to extract clusters
  void SetClusterRadius(double radius);
  void SetClusterMinNbPoints(int minNbPoints);
  vtkSetMacro(ClusterExtractor, int);

  // Set duration of gaussian model
  void SetTrackingWindowSizes(int trackingWindowSizes);

protected:
  // constructor / destructor
  vtkMotionDetector();
  ~vtkMotionDetector();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  // copy operators
  vtkMotionDetector(const vtkMotionDetector&);
  void operator=(const vtkMotionDetector&);

  // Time for initialization (frames)
  int InitNbFrames = 100;

  // Max distance to apply motion detection
  double DetectionRange[2] = { 0., 50. };

  // Parameters to subsample motion points and remove isolated motion points
  double SubsampleRange = 6.;
  double SubsampleResolution = -1.;
  double RemovalOutlierRadius = 0.1;
  int RemovalOutlierNeighbors = 10;

  // Parameters to extract clusters
  enum Extractor
  {
    NOEXTRACTION = 0,
    EUCLIDEAN = 1,
    GMM = 2,
    REGION_GROWING = 3,
  };
  int ClusterExtractor = 0;
  double ClusterRadius = 0.4;
  int ClusterMinNbPoints = 5;

  // Parameters for tracking
  int TrackingWindowSizes = 10;

  struct Voxel
  {
    // Cluster id
    int ClusterIdx = -1;

    // Point indices for current frame
    std::vector<int> CurrentPtIndices;
    int NbCurrentPts = 0;
    int SeenTimes = 0;
    int Time = -1;
  };
  struct ClusteringGrid
  {
    // Grid of voxels
    std::unordered_map<int, Voxel> VoxelMap;
    std::unordered_map<int, Voxel> BackgroudMap;
    Eigen::Vector3d Origin = { 0., 0., 0. };
    Eigen::Array3i GridSize = { 100, 100, 100 };
    float Resolution = 0.1;

    bool IsInBounds(const Eigen::Array3i& voxelId3d)
    {
      if (voxelId3d.x() >= this->GridSize.x() || voxelId3d.y() >= this->GridSize.y() ||
        voxelId3d.z() >= this->GridSize.z())
        return false;
      else
        return true;
    }

    bool Check(const Eigen::Array3i& voxelId3d)
    {
      int idx = this->To1d(voxelId3d);
      return this->VoxelMap.count(idx);
    }

    Voxel& operator()(const Eigen::Array3i& voxelId3d)
    {
      int idx = this->To1d(voxelId3d);
      return this->VoxelMap[idx];
    }

    int To1d(const Eigen::Array3i& voxelId3d) const
    {
      int id =
        voxelId3d.z() * GridSize[0] * GridSize[1] + voxelId3d.y() * GridSize[0] + voxelId3d.x();
      return id;
    }

    Eigen::Array3i To3d(int voxelId1d) const
    {
      int x = voxelId1d % this->GridSize[0];
      int y = (voxelId1d / this->GridSize[0]) % this->GridSize[1];
      int z = voxelId1d / (this->GridSize[0] * this->GridSize[1]);
      return { x, y, z };
    }
  };
  ClusteringGrid ClustersGrid;
  int NewClusterIdx = 0;

  enum Label
  {
    HUMAN = 0,
    OTHERS = 1,
  };

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

    Eigen::Matrix<double, 6, 1> GetVertices() const { return this->Vertices; }
    Eigen::Vector3d GetSize() const { return this->Size; }
    Eigen::Vector3d GetCenter() const { return this->Center; }
    std::vector<double> GetTransform() const { return this->Transform; }

  private:
    std::vector<double> Transform;
    Eigen::Matrix<double, 6, 1> Vertices = { 0., 0., 0., 0., 0., 0. };
    Eigen::Vector3d Center = { 0, 0, 0 };
    Eigen::Vector3d Size = { 0, 0, 0 };
  };
  struct ClusterStats
  {
    ClusterStats() = default;
    Label ClusterLabel = Label::HUMAN;
    int ClusterId = 0;
    int NbPoints = 0;
    double MeanDepth = 0.;
    double MeanIntensity = 0;
    Bbox BoundingBox;
  };
  std::vector<ClusterStats> ClustersStats;

  // Add a frame to the spherical map and estimate motion probability
  void EstimateMotion(vtkSmartPointer<vtkPolyData> polydata,
    vtkSmartPointer<vtkPolyData> motionPolydata);

  // Extract clusters of motion points
  void ExtractClustersWithEuclidean(vtkSmartPointer<vtkPolyData> input,
    vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
    vtkSmartPointer<vtkTable> infoOutput);

  void ExtractClustersWithGMM(vtkSmartPointer<vtkPolyData> polydata,
    vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
    vtkSmartPointer<vtkTable> infoOutput);

  // Identify input arrays to use
  bool IdentifyInputArrays(vtkPolyData* polydata);

  /**
   * Internals parameters and functions of motion detector
   * Gaussian
   * Gaussian mixture
   * Spherical depth map
   */
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  class vtkClustering;
  std::unique_ptr<vtkClustering> Clustering;
};

#endif // VTK_MOTION_DETECTOR_H
