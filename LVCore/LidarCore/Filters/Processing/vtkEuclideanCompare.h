/*=========================================================================

  Program: LidarView
  Module:  vtkEuclideanCompare.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkEuclideanCompare_h
#define vtkEuclideanCompare_h

#include "lvFiltersProcessingModule.h" // For export macro

#include <vtkAbstractPointLocator.h>
#include <vtkDataSetAlgorithm.h>
#include <vtkSmartPointer.h> // for ivar

class vtkDataSet;
class vtkPoints;
class vtkDoubleArray;

/**
 * This filter computes the Euclidean distance from each point in the target cloud
 * to its nearest neighbor in the source point cloud.
 *
 * For faster computation, the target cloud can be voxelized, note that this reduces
 * the resolution of the output scalar.
 */
class LVFILTERSPROCESSING_EXPORT vtkEuclideanCompare : public vtkDataSetAlgorithm
{
public:
  static vtkEuclideanCompare* New();
  vtkTypeMacro(vtkEuclideanCompare, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(UseVoxelGrid, bool);
  vtkGetMacro(UseVoxelGrid, bool);

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
   * Use a kdtree to find the closest point.
   * Should be enabled if the reference point cloud is static or does not changes often,
   * as building the kd tree have a time cost.
   * If disabled use a the uniform point locator instead.
   */
  vtkSetMacro(UseKDTree, bool);
  vtkGetMacro(UseKDTree, bool);
  ///@}

  ///@{
  /**
   * The name of the scalar used to store the result of the operation.
   */
  vtkSetMacro(ResultScalarName, std::string);
  vtkGetMacro(ResultScalarName, std::string);
  ///@}

protected:
  vtkEuclideanCompare();
  ~vtkEuclideanCompare() override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkEuclideanCompare(const vtkEuclideanCompare&) = delete;
  void operator=(const vtkEuclideanCompare&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void Compare(vtkPoints* refPoints, vtkDataSet* targetDs, vtkDoubleArray* outputArr);
  void CompareWithVoxelGrid(vtkPoints* refPoints, vtkDataSet* targetDs, vtkDoubleArray* outputArr);

  vtkMTimeType ReferenceLastMTime;
  vtkSmartPointer<vtkAbstractPointLocator> Locator;
  bool UseVoxelGrid = false;
  double LeafSize = 0.1;
  bool UseKDTree = true;
  std::string ResultScalarName = "Distance";
};

#endif
