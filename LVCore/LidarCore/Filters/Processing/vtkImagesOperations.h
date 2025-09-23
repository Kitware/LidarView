/*=========================================================================

  Program: LidarView
  Module:  vtkImagesOperations.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_IMAGES_OPERATIONS_H
#define VTK_IMAGES_OPERATIONS_H

// VTK
#include <vtkImageAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <Eigen/Dense>

#include "lvFiltersProcessingModule.h"

/**
 * ImagesOperations aggregate two images using a provided function which can be one of:
 * Sum, Difference, Absolute Difference, Product.
 */
class LVFILTERSPROCESSING_EXPORT vtkImagesOperations : public vtkImageAlgorithm
{
public:
  static vtkImagesOperations* New();
  vtkTypeMacro(vtkImagesOperations, vtkImageAlgorithm)

  ///@{
  /**
   * The operatation to use during images aggregation.
   */
  vtkSetMacro(Operation, int);
  vtkGetMacro(Operation, int);
  ///@}

  ///@{
  /**
   * The name of the scalar used to store the result of the operation.
   */
  vtkSetMacro(ResultScalarName, std::string);
  vtkGetMacro(ResultScalarName, std::string);
  ///@}

  // All possible operations on scalar
  enum Operator
  {
    SUM = 0,
    DIFFERENCE = 1,
    ABSOLUTE_DIFFERENCE = 2,
    PRODUCT = 3,
  };

protected:
  vtkImagesOperations();
  ~vtkImagesOperations() = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImagesOperations(const vtkImagesOperations&) = delete;
  void operator=(const vtkImagesOperations&) = delete;

  template <typename T>
  std::function<T(T, T)> GetOperation();

  // Compute operation
  template <typename ArrayT>
  void PerformImageOperation(vtkAbstractArray* inArray1,
    vtkAbstractArray* inArray2,
    vtkPointData* outPointData);

  int Operation = 0;

  // The name of the output scalar
  std::string ResultScalarName = "Result";
};

#endif // VTK_IMAGES_OPERATIONS_H
