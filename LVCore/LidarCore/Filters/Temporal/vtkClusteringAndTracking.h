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
};

#endif // VTK_CLUSTERING_H
