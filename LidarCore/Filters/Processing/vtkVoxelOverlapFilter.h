/*=========================================================================

  Program: LidarView
  Module:  vtkVoxelOverlapFilter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVoxelOverlapFilter_h
#define vtkVoxelOverlapFilter_h

#include "lvFiltersProcessingModule.h" // For export macro

#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h> // for ivar

class vtkDataSet;
class vtkFieldData;

/**
 * This filter constructs a voxel grid from a source point cloud and uses it
 * to detect differences with a target point cloud.
 *
 * It identifies points in the target cloud that fall outside the occupied voxels of the source
 * grid, effectively highlighting discrepancies or outliers between the two datasets.
 */
class LVFILTERSPROCESSING_EXPORT vtkVoxelOverlapFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkVoxelOverlapFilter* New();
  vtkTypeMacro(vtkVoxelOverlapFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Size of the leaf used to downsample the point cloud, equivalent to the average
   * resolution of the cloud.
   */
  vtkSetMacro(LeafSize, double);
  vtkGetMacro(LeafSize, double);
  ///@}

  ///@{
  /**
   * The name of the scalar used to store the result of the operation.
   */
  vtkSetMacro(ResultScalarName, std::string);
  vtkGetMacro(ResultScalarName, std::string);
  ///@}

protected:
  vtkVoxelOverlapFilter();
  ~vtkVoxelOverlapFilter() override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkVoxelOverlapFilter(const vtkVoxelOverlapFilter&) = delete;
  void operator=(const vtkVoxelOverlapFilter&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  double LeafSize = 0.1;
  std::string ResultScalarName = "IsOverlapping";
};

#endif
