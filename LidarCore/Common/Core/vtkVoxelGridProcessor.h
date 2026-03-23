/*=========================================================================

  Program:   LidarView
  Module:   vtkVoxelGridProcessor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVoxelGridProcessor_H
#define vtkVoxelGridProcessor_H

// STL includes
#include <algorithm>
#include <unordered_map>
#include <vector>

// Eigen includes
#include <Eigen/Dense>

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>

#include "lvCommonCoreModule.h"

typedef unsigned long long int uint64;

class vtkDataSet;

/**
 * @class vtkVoxelGridProcessor
 * @brief Filter a point cloud using a voxel grid.
 *
 * Filters a point cloud using a voxel grid. An unordered map is used to store the id of the points
 * in the voxel grid. A 64 bit unsigned integer is used to avoid overflow. The points are stored in
 * a vtkPolyData. The methods AddPoint() are used to add a point to the voxel grid. A vtkPolyData is
 * required as an argument of AddPoint() to copy the point data. Coordinates can also be given to
 * AddPoint() to add a custom point associated with an index of the polydata. Points from different
 * vtkPolyData can be added to the same voxel grid.
 */

class LVCOMMONCORE_EXPORT vtkVoxelGridProcessor : public vtkObject
{
public:
  static vtkVoxelGridProcessor* New();
  vtkTypeMacro(vtkVoxelGridProcessor, vtkObject);

  /**
   * Modes to downsample the points in the voxel grid
   */
  enum SamplingMode
  {
    //! Use the first point acquired
    //! Useful for performances issues
    FIRST = 0,

    //! Use the last point acquired
    //! Can be useful for specific lidar orientations
    LAST = 1,

    //! Use the point with maximum intensity
    //! The max intensity points can be the most acurate
    MAX_INTENSITY = 2,

    //! Use the center of the voxel
    //! This allows uniform sampling but is biased
    CENTER_POINT = 3,

    //! Use the centroid of the voxel
    //! /!\ This smoothes the points (intersections might be badly represented)
    //! /!\ The sampling process is longer
    CENTROID = 4
  };

  /**
   * @brief AddPoint Add a point with given coordinates to the voxel grid
   */
  bool AddPoint(vtkDataSet* points, vtkIdType id, const double coord[3]);
  /**
   * @brief AddPoint Add a point from a polydata to the voxel grid
   */
  bool AddPoint(vtkDataSet* points, vtkIdType id);

  /**
   * Add all point contained in a vtkDataSet
   */
  void AddPoints(vtkDataSet* points);

  /**
   * Returns the corresponding voxel grid id for a coordinate
   */
  uint64_t GetPointId(const double coord[3]);

  /**
   * Returns true if the voxel of this coord is already filled
   */
  bool HasPointInVoxel(const double coord[3]);

  /**
   * @brief ResizeData Check if enough memory is allocated to store the points, resize data if
   * necessary in order to use InsertNextPoint() without reallocation
   */
  bool ResizeData();

  /**
   * @brief FreeUnusedMemory Free up extra allocated memory
   */
  void FreeUnusedMemory();

  /**
   * @brief Clear Remove all points from all voxels and clear the voxel grid
   */
  void Clear();

  vtkGetMacro(Sampling, SamplingMode);
  vtkSetMacro(Sampling, SamplingMode);
  void SetSampling(int mode) { this->SetSampling(static_cast<SamplingMode>(mode)); }

  vtkGetMacro(InitialNumberOfPoints, int);
  /**
   * @brief SetInitialNumberOfPoints Set the initial number of points
   * to allocate and initialize the data
   */
  void SetInitialNumberOfPoints(int nbPoints);

  vtkGetMacro(ResizeNumberOfPoints, int);
  vtkSetMacro(ResizeNumberOfPoints, int);

  vtkGetVector6Macro(Bounds, double);
  /**
   * @brief SetBounds Set the bounds of the voxel grid and update the voxel grid
   */
  void SetBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  void SetBounds(double bounds[6])
  {
    this->SetBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
  }

  vtkGetMacro(LeafSize, double);
  void SetLeafSize(double ls);

  /**
   * @brief GetVoxelGrid Return the voxel grid of indices
   */
  const std::unordered_map<uint64, vtkIdType>& GetVoxelGrid() { return this->VoxelGrid; }

  /**
   * @brief GetOutput Return the output point cloud
   */
  vtkPolyData* GetOutput() const { return this->Output; }

  /**
   * @brief GetFilteredOutput Return the output point cloud filtered on minimum frames number per
   * voxel grid
   */
  vtkSmartPointer<vtkPolyData> GetFilteredOutput(int minNumber);

protected:
  vtkVoxelGridProcessor();

  /**
   * @brief To1d Convert 3D voxel index to 1D flattened index
   */
  uint64 To1d(const Eigen::Array3i& voxelId3d) const;

  /**
   * @brief To3d Convert 1D flattened voxel index to 3D index
   */
  Eigen::Array3i To3d(uint64 voxelId1d) const;

  /**
   * @brief UpdateVoxelSize Update the size of the voxel grid with Bounds and LeafSize
   * Check potential overflow
   */
  void UpdateVoxelSize();

  /**
   * @brief RecomputeVoxelGrid Resize the voxel grid data. Should be called after UpdateVoxelSize()
   * to copy existing data to the new voxel grid
   */
  void RecomputeVoxelGrid();

  /**
   * @brief ExpandVoxelGrid Expand the voxel grid if the previous bounds are contained in the new
   * ones and update Bounds with the exact new bounds of the voxel grid
   */
  void ExpandVoxelGrid(const double previousBounds[6], const uint64 previousGridSize[3]);

  /**
   * @brief InitializeData Initialize data structures : initialize Output by allocating
   * InitialNumberOfPoints to the vtkPoints and vtkCells and set the associated counters
   */
  void InitializeData();

  /**
   * @brief PositionToVoxelId Convert a 3D position to a 1D flattened voxel index
   */
  uint64 PositionToVoxelId(const Eigen::Vector3d& position) const;

  /**
   * @brief VoxelIdToPosition Give the center of a voxel from its 1D flattened index
   */
  Eigen::Vector3d VoxelIdToPosition(uint64 voxelId) const;

  /**
   * @brief IsInBounds Check if a 3D position is inside the voxel grid
   */
  bool IsInBounds(const Eigen::Vector3d& position) const;

  //! [m] Size of the leaf used to downsample the pointcloud,
  //! it will be equivalent to the average resolution of the output pointcloud
  double LeafSize = 0.2;

  //! Size of the voxel grid, this parameter is automatically computed from the bounds and the leaf
  //! size
  uint64 GridSize[3] = { 100, 100, 100 };

  //! Map from 1D flattened voxel index to the index of the corresponding point in Output
  std::unordered_map<uint64, vtkIdType> VoxelGrid;

  //! Store the points of the voxel grid
  vtkNew<vtkPolyData> Output;

  //! Store the number of points in each voxel for the CENTROID sampling mode
  std::vector<int> NumberOfPointsPerVoxel;

  //! The grid is filtered to contain at most one point per voxel
  //! This mode parameter allows to choose how to select the remaining point
  //! It can be : taking the first/last acquired point, taking the max intensity point,
  //! considering the closest point to the voxel center or averaging the points.
  SamplingMode Sampling = SamplingMode::FIRST;

  //! Bounds of the voxel grid
  double Bounds[6] = { -400.0, 400.0, -400.0, 400.0, -20.0, 50.0 };

  //! Number of additional points to allocate when resizing the voxel grid
  int ResizeNumberOfPoints = 50000;
  //! Number of points to allocate at the beginning
  int InitialNumberOfPoints = 100000;

private:
  vtkVoxelGridProcessor(const vtkVoxelGridProcessor&);
  void operator=(const vtkVoxelGridProcessor&);

  //! Number of voxels in the voxel grid, necessary to free unused memory
  vtkIdType VoxelCount = 0;

  //! Flag to know if the voxel grid has been initialized
  bool VoxelPointDataInitialized = false;

  //! Flag to know if the voxel grid is valid
  //! If the voxel grid is not valid, the points are added to the output without being added to the
  //! voxel grid
  bool IsVoxelGridValid = true;

  //! Number of points allocated in the output
  int CurrentDataSize = 0;
};

#endif // vtkVoxelGridProcessor_H
