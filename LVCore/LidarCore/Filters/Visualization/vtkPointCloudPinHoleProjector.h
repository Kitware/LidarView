/*=========================================================================

  Program: LidarView
  Module:  vtkPointCloudPinHoleProjector.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_POINTCLOUD_PINHOLE_PROJECTOR_H
#define VTK_POINTCLOUD_PINHOLE_PROJECTOR_H

// VTK
#include <vtkImageAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <Eigen/Dense>

#include "lvFiltersVisualizationModule.h"

/**
 * PointCloudPinHoleProjector rasterize a point cloud on an imageData using a PinHole model.
 */
class LVFILTERSVISUALIZATION_EXPORT vtkPointCloudPinHoleProjector : public vtkImageAlgorithm
{
public:
  static vtkPointCloudPinHoleProjector* New();
  vtkTypeMacro(vtkPointCloudPinHoleProjector, vtkImageAlgorithm);

  ///@{
  /**
   * The resolution of the output image.
   */
  vtkGetVector2Macro(Resolution, unsigned int);
  vtkSetVector2Macro(Resolution, unsigned int);
  ///@}

  ///@{
  /**
   * The position of the camera.
   */
  void SetCameraTranslation(double x, double y, double z);
  Eigen::Vector3d GetCameraTranslation();
  ///@}

  ///@{
  /**
   * The rotation of the camera, given by a directional vector.
   * The given vector will be normalized, magnitude doesn't matter.
   */
  void SetCameraDirection(double x, double y, double z);
  Eigen::Vector3d GetCameraDirection();
  ///@}

  ///@{
  /**
   * The angle made by the camera around the directional vector.
   */
  void SetCameraAngle(double angle);
  double GetCameraAngle();
  ///@}

  ///@{
  /**
   * Whether or not to use NaN to initalize double scalar values.
   */
  vtkGetMacro(UseNaN, bool);
  vtkSetMacro(UseNaN, bool);
  ///@}

protected:
  vtkPointCloudPinHoleProjector();
  ~vtkPointCloudPinHoleProjector() = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPointCloudPinHoleProjector(const vtkPointCloudPinHoleProjector&) = delete;
  void operator=(const vtkPointCloudPinHoleProjector&) = delete;

  unsigned int Resolution[2] = { 1280, 720 };
  Eigen::Vector3d CameraTranslation = Eigen::Vector3d(0, 0, 0);
  Eigen::Vector3d CameraDirection = Eigen::Vector3d(1, 0, 0);
  double CameraAngle = 0;
  bool UseNaN = false;

  Eigen::Matrix3d RotationMatrix = Eigen::Matrix3d::Identity();

  // Rasterize a given polydata into a new vtkImageData.
  vtkSmartPointer<vtkImageData> polyDataToImageData(vtkSmartPointer<vtkPolyData> input);

  // Update the transformation matrix, called when camera related properties are updated.
  void UpdateTransform();

  // Create a 3d model to indicate to the user camera location/rotation.
  vtkSmartPointer<vtkPolyData> generateCameraModel();
};

#endif // VTK_POINTCLOUD_PINHOLE_PROJECTOR_H
