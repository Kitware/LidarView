/*=========================================================================

  Program: LidarView
  Module:  vtkPointsPCA.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPointsPCA_h
#define vtkPointsPCA_h

// VTK includes
#include <vtkDataSetAlgorithm.h>
#include <vtkSmartPointer.h>

#include "lvCommonCoreModule.h" // for export macro

class vtkPCAStatistics;
class vtkVector3d;
class vtkDoubleArray;

/**
 * vtkPointsPCA compute the PCA (using vtkPCAStatistic class) on input
 * dataset points.
 */
class LVCOMMONCORE_EXPORT vtkPointsPCA : public vtkDataSetAlgorithm
{
public:
  static vtkPointsPCA* New();
  vtkTypeMacro(vtkPointsPCA, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get eigen values. The eigen values are ordered according from largest to smallest.
   */
  void GetEigenValues(vtkDoubleArray*);
  vtkVector3d GetEigenValues();
  double GetEigenValue(int i);
  ///@}

  ///@{
  /**
   *  Set point indices to use in PCA
   */
  void SetPointIndices(const std::vector<int>& pointIndices);
  ///@}

  ///@{
  /**
   * Get eigen vectors. The eigenvectors are ordered according to the magnitude of their
   * associated eigen values, sorted from largest to smallest. That is, eigenvector 0 corresponds
   * to the largest eigen value.
   */
  void GetEigenVectors(vtkDoubleArray*);
  void GetEigenVector(int i, vtkDoubleArray*);
  vtkVector3d GetEigenVector(int i);
  ///@}

protected:
  vtkPointsPCA();
  ~vtkPointsPCA() = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPointsPCA(const vtkPointsPCA&);
  void operator=(const vtkPointsPCA&);

  vtkSmartPointer<vtkPCAStatistics> PCA;
  std::vector<int> PointIndices;
};

#endif // vtkPointsPCA_h
