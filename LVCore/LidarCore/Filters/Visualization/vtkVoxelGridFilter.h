/*=========================================================================

  Program:   LidarView
  Module:    vtkVoxelGridFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVoxelGridFilter_h
#define vtkVoxelGridFilter_h

// vtk includes
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include "lvFiltersVisualizationModule.h"

/**
 * @class vtkVoxelGridFilter
 * @brief The vtkVoxelGridFilter class filters points with a uniform voxel grid
 * (vtkVoxelGridProcessor)
 */
class LVFILTERSVISUALIZATION_EXPORT vtkVoxelGridFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkVoxelGridFilter* New();
  vtkTypeMacro(vtkVoxelGridFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(LeafSize, double);
  vtkGetMacro(LeafSize, double);

  vtkSetMacro(SamplingMode, int);
  vtkGetMacro(SamplingMode, int);

protected:
  // constructor / destructor
  vtkVoxelGridFilter() = default;
  ~vtkVoxelGridFilter() = default;

  // Request data
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Leaf size
  double LeafSize = 0.5;

  // Specify how to sample the points within a leaf
  int SamplingMode = 0;

private:
  // copy operators
  vtkVoxelGridFilter(const vtkVoxelGridFilter&);
  void operator=(const vtkVoxelGridFilter&);
};

#endif // vtkVoxelGridFilter_h
