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

#include "NeighborsSearch.h"

//----------------------------------------------------------------------------
NeighborsSearch::NeighborsSearch(vtkSmartPointer<vtkPolyData> cloud, Search::Mode mode)
  : Mode(mode)
{
  this->SetCloud(cloud);
}

//----------------------------------------------------------------------------
NeighborsSearch::NeighborsSearch(vtkSmartPointer<vtkPolyData> cloud, bool search)
{
  this->SetCloud(cloud, search);
}

//----------------------------------------------------------------------------
void NeighborsSearch::SetCloud(vtkSmartPointer<vtkPolyData> cloud, bool search)
{
  this->Cloud = cloud;
  if (search)
  {
    this->Tree->Reset(this->Cloud);
  }
}

//----------------------------------------------------------------------------
void NeighborsSearch::SearchNeighbors(const float pt[3], std::vector<int>& neighborsIndices)
{
  Eigen::Vector3f ptEigen(pt[0], pt[1], pt[2]);

  switch (this->Mode)
  {
    case Search::KNN:
    {
      this->SelectNeighborsKnn(pt, this->Knn, neighborsIndices);

      double vtkFurthestPoint[3];
      this->Cloud->GetPoint(neighborsIndices.back(), vtkFurthestPoint);
      Eigen::Vector3f furthestPoint(static_cast<float>(vtkFurthestPoint[0]),
        static_cast<float>(vtkFurthestPoint[1]),
        static_cast<float>(vtkFurthestPoint[2]));

      if ((ptEigen - furthestPoint).norm() < this->Radius)
      {
        this->SelectNeighborsRadius(pt, this->Radius, neighborsIndices);
      }
      break;
    }

    default:
    {
      this->SelectNeighborsRadius(pt, this->Radius, neighborsIndices);

      if (neighborsIndices.size() < this->Knn)
      {
        this->SelectNeighborsKnn(pt, this->Knn, neighborsIndices);
      }
    }
  }
}

//----------------------------------------------------------------------------
void NeighborsSearch::SearchNeighbors(int ref, std::vector<int>& neighborsIndices)
{
  double vtkPt[3];
  this->Cloud->GetPoint(ref, vtkPt);
  const float pt[3] = {
    static_cast<float>(vtkPt[0]), static_cast<float>(vtkPt[1]), static_cast<float>(vtkPt[2])
  };
  Eigen::Vector3f ptEigen(pt[0], pt[1], pt[2]);

  switch (this->Mode)
  {
    case Search::KNN:
    {
      this->SelectNeighborsKnn(pt, this->Knn + 1, neighborsIndices);

      double vtkFurthestPoint[3];
      this->Cloud->GetPoint(neighborsIndices.back(), vtkFurthestPoint);
      Eigen::Vector3f furthestPoint(static_cast<float>(vtkFurthestPoint[0]),
        static_cast<float>(vtkFurthestPoint[1]),
        static_cast<float>(vtkFurthestPoint[2]));

      if ((ptEigen - furthestPoint).norm() < this->Radius)
      {
        this->SelectNeighborsRadius(pt, this->Radius, neighborsIndices);
      }
      break;
    }

    default:
    {
      this->SelectNeighborsRadius(pt, this->Radius, neighborsIndices);

      if (neighborsIndices.size() < this->Knn + 1)
      {
        this->SelectNeighborsKnn(pt, this->Knn + 1, neighborsIndices);
      }
    }
  }
  neighborsIndices = { neighborsIndices.begin() + 1, neighborsIndices.end() };
}

//----------------------------------------------------------------------------
void NeighborsSearch::SelectNeighborsRadius(const float pt[3],
  float radius,
  std::vector<int>& neighborsIndices)
{
  std::vector<float> distances;
  std::vector<int> neighbors;

  this->Tree->radiusSearch(pt, radius * radius, neighbors, distances);
  neighborsIndices = { neighbors.begin() + 1, neighbors.end() };

  if (neighborsIndices.size() < this->Knn)
  {
    neighborsIndices.clear();
    neighborsIndices.reserve(this->Knn + 1);

    std::vector<float> distances;
    this->Tree->KnnSearch(pt, this->Knn + 1, neighborsIndices, distances);
    neighborsIndices = { neighborsIndices.begin() + 1, neighborsIndices.end() };
  }
}

//----------------------------------------------------------------------------
void NeighborsSearch::SelectNeighborsKnn(const float pt[3],
  int knn,
  std::vector<int>& neighborsIndices)
{
  std::vector<float> distances;
  this->Tree->KnnSearch(pt, knn, neighborsIndices, distances);
}
