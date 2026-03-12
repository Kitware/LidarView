/*=========================================================================

  Program:   LidarView
  Module:   vtkMergePointsToPolyDataHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMergePointsToPolyDataHelper_H
#define vtkMergePointsToPolyDataHelper_H

// STL includes
#include <algorithm>
#include <vector>

// Eigen includes
#include <Eigen/Dense>

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>

#include "lvCommonCoreModule.h"

/**
 * @class vtkMergePointsToPolyDataHelper
 * @brief Helper class to merge points from different vtkPolyData into a single vtkPolyData with the
 * same PointData
 *
 * The points are stored in a vtkPolyData. The methods AddPoint() are used to add a point to the
 * output. A vtkPolyData is required as an argument of AddPoint() to copy the point data.
 * Coordinates can also be given to AddPoint() to add a custom point associated with an index of the
 * polydata. Points from different vtkPolyData can be added to the output.
 */

class LVCOMMONCORE_EXPORT vtkMergePointsToPolyDataHelper : public vtkObject
{
public:
  static vtkMergePointsToPolyDataHelper* New();
  vtkTypeMacro(vtkMergePointsToPolyDataHelper, vtkObject);
  /**
   * @brief AddPoint Add a point with given coordinates
   */
  bool AddPoint(vtkPolyData* points, vtkIdType id, const double coord[3]);
  /**
   * @brief AddPoint Add a point from a polydata
   */
  bool AddPoint(vtkPolyData* points, vtkIdType id);

  /**
   * @brief ResizeData Check if enough memory is allocated to store the points, resize data if
   * necessary
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

  vtkGetMacro(InitialNumberOfPoints, int);
  /**
   * @brief SetInitialNumberOfPoints Set the initial number of points
   * to allocate and initialize the data
   */
  void SetInitialNumberOfPoints(int nbPoints);

  vtkGetMacro(ResizeNumberOfPoints, int);
  vtkSetMacro(ResizeNumberOfPoints, int);

  /**
   * @brief GetOutput Return the output point cloud
   */
  vtkPolyData* GetOutput() const { return this->Output; }

protected:
  vtkMergePointsToPolyDataHelper() = default;

  /**
   * @brief InitializeData Initialize the output
   */
  void InitializeData();

  //! Store the points
  vtkNew<vtkPolyData> Output;

  //! Number of additional points to allocate when the number of points is exceeded
  int ResizeNumberOfPoints = 50000;
  //! Number of points to allocate at the beginning
  int InitialNumberOfPoints = 100000;

private:
  vtkMergePointsToPolyDataHelper(const vtkMergePointsToPolyDataHelper&);
  void operator=(const vtkMergePointsToPolyDataHelper&);

  //! Flag to know if the output has been initialized
  bool OutputInitialized = false;

  //! Number of points allocated in the output
  int CurrentDataSize = 0;
};

#endif // vtkMergePointsToPolyDataHelper_H
