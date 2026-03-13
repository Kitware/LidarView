/*=========================================================================

  Program: LidarView
  Module:  NeighborsSearch.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef NEIGHBORS_SEARCH
#define NEIGHBORS_SEARCH

#include <vector>

#include "Eigen/Core"
#include "Eigen/Dense"

// VTK includes
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include "KDTreeVTKAdaptor.h"

namespace Search
{
typedef enum
{
  KNN = 0,
  RADIUS = 1
} Mode;
}

/**
 * NeighborsSearch utilizes nanoflann to search for points in the neighborhood of a given
 * point, either by finding the k nearest neighbors or by identifying points within a specified
 * radius around the given point
 */
class NeighborsSearch
{
public:
  NeighborsSearch() = default;

  NeighborsSearch(vtkSmartPointer<vtkPolyData> cloud, Search::Mode mode);

  NeighborsSearch(vtkSmartPointer<vtkPolyData> cloud, bool search = true);

  // Set cloud
  void SetCloud(vtkSmartPointer<vtkPolyData> cloud, bool search = true);

  // Set radius
  void SetRadius(float radius) { this->Radius = radius; }

  // Set k nearest neighbor
  void SetKnn(unsigned int knn) { this->Knn = knn; }

  // Search neighbors using point
  void SearchNeighbors(const float pt[3], std::vector<int>& neighborsIndices);

  // Search neighbors using ref of point
  void SearchNeighbors(int ref, std::vector<int>& neighborsIndices);

  // Return size of points cloud
  int size() { return this->Cloud->GetNumberOfPoints(); }

  /* // Overload subscript operator []
  Point& operator[](int idx) {return this->Cloud->at(idx);}*/

  // Overload at(i) function
  double* at(int idx) { return this->Cloud->GetPoint(idx); }

  // Get pointer to cloud
  vtkSmartPointer<vtkPolyData> GetCloud() { return this->Cloud; }

private:
  // Search radius
  double Radius{ 1e-5 };

  //----------------------------------------------------------------------------------------------------------------
  // Search K nearest neighbors
  unsigned int Knn{ 5 };

  //----------------------------------------------------------------------------------------------------------------
  // Controls how the closest neighbor is calculated
  Search::Mode Mode;

  //----------------------------------------------------------------------------------------------------------------
  // Input point cloud
  vtkSmartPointer<vtkPolyData> Cloud{ vtkSmartPointer<vtkPolyData>::New() };

  //----------------------------------------------------------------------------------------------------------------
  // Kd tree built upon input point cloud
  std::shared_ptr<vtkKDTreeVTKAdaptor> Tree{ new vtkKDTreeVTKAdaptor };

  //----------------------------------------------------------------------------------------------------------------
  // Extract the nearest neighbors in a radius of a query point
  void SelectNeighborsRadius(const float pt[3], float radius, std::vector<int>& neighborsIndices);

  //----------------------------------------------------------------------------------------------------------------
  // Extract the knn nearest neighbors of a query point
  void SelectNeighborsKnn(const float pt[3], int knn, std::vector<int>& neighborsIndices);
};

#endif // NEIGHBORS_SEARCH