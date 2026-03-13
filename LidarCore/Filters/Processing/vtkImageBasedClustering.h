/*=========================================================================

  Program: LidarView
  Module:  vtkImageBasedClustering.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_IMAGE_BASED_CLUSTERING_H
#define VTK_IMAGE_BASED_CLUSTERING_H

// VTK
#include <vtkImageAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include "lvFiltersProcessingModule.h"

/**
 * vtkImageBasedClustering compare two point clouds and create clusters using 2D projection and
 * image processing.
 */
class LVFILTERSPROCESSING_EXPORT vtkImageBasedClustering : public vtkImageAlgorithm
{
public:
  static vtkImageBasedClustering* New();
  vtkTypeMacro(vtkImageBasedClustering, vtkImageAlgorithm)

  ///@{
  /**
   * The operatation to use during images aggregation.
   */
  vtkGetVector2Macro(Resolution, unsigned int);
  vtkSetVector2Macro(Resolution, unsigned int);
  ///@}

  ///@{
  /**
   * A scalar multiplicated by resolution and represent the focal distance.
   */
  vtkGetMacro(FocalLength, double);
  vtkSetMacro(FocalLength, double);
  ///@}

  ///@{
  /**
   * The minimum number of pixels to have in one cluster.
   */
  vtkGetMacro(MinNumberOfPixels, unsigned long);
  vtkSetMacro(MinNumberOfPixels, unsigned long);
  ///@}

  ///@{
  /**
   * The distance threshold to include points that disapeared during erosion process.
   */
  vtkGetMacro(MaxDistToErosionCluster, double);
  vtkSetMacro(MaxDistToErosionCluster, double);
  ///@}

  ///@{
  /**
   * The operatation to use during images aggregation.
   */
  vtkGetVector3Macro(CameraTranslation, double);
  vtkSetVector3Macro(CameraTranslation, double);
  ///@}

  ///@{
  /**
   * The operatation to use during images aggregation.
   */
  vtkGetVector3Macro(CameraDirection, double);
  vtkSetVector3Macro(CameraDirection, double);
  ///@}

  ///@{
  /**
   * The operatation to use during images aggregation.
   */
  vtkGetMacro(CameraAngle, double);
  vtkSetMacro(CameraAngle, double);
  ///@}

  ///@{
  /**
   * The operatation to use during images aggregation.
   */
  vtkGetMacro(ResultScalar, std::string);
  vtkSetMacro(ResultScalar, std::string);
  ///@}

  ///@{
  /**
   * Replace NaN values by one of its 4 neighbors if one is different from NaN.
   */
  vtkGetMacro(FillWithNeighbors, bool);
  vtkSetMacro(FillWithNeighbors, bool);
  ///@}

protected:
  vtkImageBasedClustering();
  ~vtkImageBasedClustering() = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageBasedClustering(const vtkImageBasedClustering&) = delete;
  void operator=(const vtkImageBasedClustering&) = delete;

  unsigned int Resolution[2] = { 750, 750 };
  double FocalLength = 1.0;
  bool FillWithNeighbors = false;

  int MinNumberOfPixels = 10;
  double MaxDistToErosionCluster = 0.5;

  double CameraTranslation[3] = { 0, 0, 0 };
  double CameraDirection[3] = { 1, 0, 0 };
  double CameraAngle = 0;

  std::string ResultScalar = "Cluster Id";
};

#endif // VTK_IMAGE_BASED_CLUSTERING_H
