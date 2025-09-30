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

  // Possible operations on scalar
  enum OperationType : unsigned int
  {
    SUM = 0,
    DIFF = 1,
    ABSOLUTE_DIFF = 2,
    PRODUCT = 3,

    Size
  };

  ///@{
  /**
   * The operatation to use during images aggregation.
   */
  vtkSetClampMacro(Operation, unsigned int, 0, OperationType::Size);
  vtkGetMacro(Operation, unsigned int);
  ///@}

  ///@{
  /**
   * The name of the scalar used to store the result of the operation.
   */
  vtkSetMacro(ResultScalarName, std::string);
  vtkGetMacro(ResultScalarName, std::string);
  ///@}

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

  unsigned int Operation = 0;

  // The name of the output scalar
  std::string ResultScalarName = "Result";
};

#endif // VTK_IMAGES_OPERATIONS_H
