//=========================================================================
//
// Copyright 2020 Kitware, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

#ifndef VTK_CAMERA_MAPPER_H
#define VTK_CAMERA_MAPPER_H

// VTK
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include "Common/vtkCustomTransformInterpolator.h"

// EIGEN
#include <Eigen/Dense>

// LOCAL
#include "CameraModel.h"

#include "LidarCoreModule.h"

//! Map 3D points to their projection using a camera model. The output contains
//! texture coordinates that can be used for texture mapping

class LIDARCORE_EXPORT vtkCameraMapper : public vtkPolyDataAlgorithm
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  static vtkCameraMapper *New();
  vtkTypeMacro(vtkCameraMapper, vtkPolyDataAlgorithm)

  void SetCalibrationFileName(const std::string &argfilename);
  void SetImgSize(double width, double height)
  {
    this->ImgSize(0) = width; this->ImgSize(1) = height;
    this->Modified();
  };

  vtkSetMacro(UseTrajectoryToCorrectPoints, bool);
  vtkSetMacro(UseTrajectoryToCompensateCameraMovement, bool);
  vtkSetMacro(PipelineTimeToLidarTime, double);

  void SetCameraMask(const std::string &argfilename);
  vtkSetMacro(UseCameraMask, bool);
  vtkSetMacro(InvertCameraMask, bool);

protected:
  vtkCameraMapper();

  int FillInputPortInformation(int port, vtkInformation *info) override;
  int RequestUpdateExtent(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  void Modified() override;

private:
  vtkCameraMapper(const vtkCameraMapper&) = delete;
  void operator=(const vtkCameraMapper&) = delete;

  //! Camera model
  CameraModel Model;
  double CurrentImagePipelineTime = 0.0; // time of the image given to RequestData

  //! Trajectory (TemporalTransforms) of the sensor that produced the point cloud
  //! or Trajectory followed by the camera (if UseTrajectoryToCompensateCameraMovement is on).
  vtkSmartPointer<vtkCustomTransformInterpolator> Trajectory = nullptr;

  //! File containing the camera model and parameters
  std::string CalibrationFileName;

  //! Should the cache be refreshed before next projection ?
  bool NeedsToUpdateCachedValues = false;

  //! Should the trajectory (if available) be used to correct the point
  //! coordinates ?
  //! - take into account the fact that the lidar frame sampling is different
  //! than the camera frame sampling
  //! - undistortion (lidar frame capturing is not instantaneous)
  bool UseTrajectoryToCorrectPoints = true;

  //! Should the trajectory (if available) be used to correct the camera pose?
  //! the trajectory (poses) will be used to compensate the camera movement.
  //! This can be used to map the camera with a point cloud that has already been
  //! transformed using this trajectory.
  bool UseTrajectoryToCompensateCameraMovement = true;

  //! Timeshift to apply to the pipeline time to get the lidar time.
  //! This value could be computed on the current frame, but it much better to
  //! compute an estimator such as the median using all lidar frames
  //! used only if a trajectory is provided and UseTrajectoryToCorrectPoints is
  //! true
  double PipelineTimeToLidarTime = 0.0;

  //! Image size used to create relative coordinates from pixel coordinates in
  //! case the image input not provided
  Eigen::Vector2d ImgSize = Eigen::Vector2d(1920., 1080.);

  //! Should we use a mask to invalidate some of the mapped points
  //! This is typically used in case part of the camera field of view is occluded
  //! For example a camera seeing egovehicle pixels that don't correspond to
  //! lidar points
  bool UseCameraMask = false;

  //! Camera mask Path to the image file used as texture mask (the image must be the same size
  // as the texture image source)
  vtkSmartPointer<vtkImageData> CameraMask;

  //! Should we invert mask values?
  bool InvertCameraMask = false;
};

#endif // VTK_CAMERA_MAPPER_H
