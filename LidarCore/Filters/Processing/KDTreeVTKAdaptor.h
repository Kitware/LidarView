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

#ifndef KDTREE_VTK_ADAPTOUR
#define KDTREE_VTK_ADAPTOUR

#include <nanoflann.hpp>

// VTK includes
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include <vtkDataArray.h>
#include <vtkPointData.h>

class vtkKDTreeVTKAdaptor
{

  using metric_t =
    typename nanoflann::metric_L2_Simple::traits<float, vtkKDTreeVTKAdaptor>::distance_t;
  using index_t = nanoflann::KDTreeSingleIndexAdaptor<metric_t, vtkKDTreeVTKAdaptor, -1, int>;

public:
  /**
   * \brief Build a Kd-tree from a given vtkPolyData.
   * \param cloud The vtkPolyData to encode in the kd-tree.
   * \param leafMaxSize The maximum size of a leaf of the tree (refer to
   * https://github.com/jlblancoc/nanoflann#21-kdtreesingleindexadaptorparamsleaf_max_size)
   */
  vtkKDTreeVTKAdaptor(
    vtkSmartPointer<vtkPolyData> cloud = vtkSmartPointer(vtkSmartPointer<vtkPolyData>::New()),
    const std::vector<std::string>& extraDims = {},
    const std::vector<float>& weights = {},
    int leafMaxSize = 16)
  {

    this->Reset(cloud, extraDims, weights, leafMaxSize);
  }

  /**
   * \brief Init the Kd-tree from a given vtkPolyData.
   * \param cloud The vtkPolyData to encode in the kd-tree.
   * \param leafMaxSize The maximum size of a leaf of the tree (refer to
   * https://github.com/jlblancoc/nanoflann#21-kdtreesingleindexadaptorparamsleaf_max_size)
   */
  void Reset(
    vtkSmartPointer<vtkPolyData> cloud = vtkSmartPointer(vtkSmartPointer<vtkPolyData>::New()),
    const std::vector<std::string>& extraDims = {},
    const std::vector<float>& weights = {},
    int leafMaxSize = 16)
  {
    // Copy the input cloud
    this->Cloud = cloud;
    this->ExtraDimensions = extraDims;
    this->Weights = weights;

    const size_t dim = this->GetPointDimension();
    // Build KD-tree
    this->Index =
      std::make_unique<index_t>(dim, *this, nanoflann::KDTreeSingleIndexAdaptorParams(leafMaxSize));
    this->Index->buildIndex();
  }
  /**
   * \brief Finds the `K` nearest neighbors points in the KD-tree to a given query point.
   * \param[in] queryPoint Input point to look closest neighbors to.
   * \param[in] knearest Number of nearest neighbors to find.
   * \param[out] knnIndices Indices of the NN.
   * \param[out] knnSqDistances Squared distances of the NN to the query point.
   * \return Number `N` of neighbors found.
   *
   * \note Only the first `N` entries in `knnIndices` and `knnSqDistances` will
   * be valid. Return may be less than `knearest` only if the number of
   * elements in the tree is less than `knearest`.
   */
  inline size_t KnnSearch(const float queryPoint[],
    int knearest,
    int* knnIndices,
    float* knnSqDistances) const
  {
    return this->Index->knnSearch(queryPoint, knearest, knnIndices, knnSqDistances);
  }
  inline size_t KnnSearch(const float queryPoint[],
    int knearest,
    std::vector<int>& knnIndices,
    std::vector<float>& knnSqDistances) const
  {
    // Init result to have large enough buffers that will be filled by knnSearch
    knnIndices.resize(knearest);
    knnSqDistances.resize(knearest);
    // Find nearest neighbors
    int* i = knnIndices.data();
    float* d = knnSqDistances.data();
    size_t kneighbors = this->KnnSearch(queryPoint, knearest, i, d);
    // If less than 'knearest' NN have been found, the last neighbors values are
    // wrong, therefore we need to ignore them
    knnIndices.resize(kneighbors);
    knnSqDistances.resize(kneighbors);
    return kneighbors;
  }
  inline size_t KnnSearch(const double queryPoint[],
    int knearest,
    std::vector<int>& knnIndices,
    std::vector<float>& knnSqDistances) const
  {
    float pt[3];
    std::copy(queryPoint, queryPoint + 3, pt);
    return this->KnnSearch(pt, knearest, knnIndices, knnSqDistances);
  }
  size_t radiusSearch(const float queryPoint[],
    float radius,
    std::vector<int>& knnIndices,
    std::vector<float>& knnDistances) const
  {
    std::vector<nanoflann::ResultItem<int, float>> indicesAndDistances;
    size_t numberNeighbors = this->Index->radiusSearch(queryPoint, radius, indicesAndDistances);

    for (auto& element : indicesAndDistances)
    {
      knnIndices.emplace_back(element.first);
      knnDistances.emplace_back(element.second);
    }
    return numberNeighbors;
  }
  /**
   * \brief Get the input pointcloud.
   * \return The input pointcloud used to build KD-tree.
   */
  inline vtkSmartPointer<vtkPolyData> GetInputCloud() const { return this->Cloud; }

  size_t GetPointDimension() const { return 3 + this->ExtraDimensions.size(); }

  // ---------------------------------------------------------------------------
  //   Methods required by nanoflann adaptor design
  // ---------------------------------------------------------------------------

  inline const vtkKDTreeVTKAdaptor& derived() const { return *this; }

  inline vtkKDTreeVTKAdaptor& derived() { return *this; }

  /**
   * \brief Returns the number of points in the input pointcloud.
   * \note This method is required by nanoflann design, and should not be used
   * by user.
   */
  inline size_t kdtree_get_point_count() const { return this->Cloud->GetNumberOfPoints(); }
  /**
   * \brief Returns the dim'th component of the idx'th point of the pointcloud.
   * \note `dim` should only be in range [0-3].
   * \note This method is required by nanoflann design, and should not be used
   * by user.
   */
  inline float kdtree_get_pt(const int idx, const int dim) const
  {
    float weight = int(this->Weights.size()) <= dim ? 0.0 : this->Weights[dim];
    if (dim < 3)
    {
      double point[3];
      this->Cloud->GetPoint(idx, point);
      return static_cast<float>(point[dim]) * weight;
    }
    else
    {
      int extraIdx = dim - 3;
      if (extraIdx >= int(this->ExtraDimensions.size()))
        return 0.f;
      vtkDataArray* arr =
        this->Cloud->GetPointData()->GetArray(this->ExtraDimensions[extraIdx].c_str());
      if (!arr)
        return 0.f;
      return static_cast<float>(arr->GetTuple1(idx)) * weight;
    }
  }
  /**
   * Optional bounding-box computation.
   * Return false to default to a standard bbox computation loop.
   * Return true if the BBOX was already computed by the class and returned in
   * "bb" so it can be avoided to redo it again.
   * Look at bb.size() to find out the expected dimensionality.
   * \note This method is required by nanoflann design, and should not be used
   * by user.
   */
  template <class BBOX>
  inline bool kdtree_get_bbox(BBOX& /*bb*/) const
  {
    return false;
  }

protected:
  //! The kd-tree index for the user to call its methods as usual with any other FLANN index.
  std::unique_ptr<index_t> Index;

  //! The input data
  vtkSmartPointer<vtkPolyData> Cloud;

  //! The array names of extra dimensions
  std::vector<std::string> ExtraDimensions;

  //! Weights for each dimension
  std::vector<float> Weights;
};

#endif // KDTREE_VTK_ADAPTOUR