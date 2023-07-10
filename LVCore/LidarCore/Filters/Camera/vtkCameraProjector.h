//=========================================================================
//
// Copyright 2019 Kitware, Inc.
// Author: Guilbert Pierre (spguilbert@gmail.com)
// Data: 03-27-2019
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

#ifndef VTK_CAMERA_PROJECTOR_H
#define VTK_CAMERA_PROJECTOR_H

// VTK
#include <vtkImageAlgorithm.h>
#include <vtkSmartPointer.h>

// EIGEN
#include <Eigen/Dense>

// LOCAL
#include "CameraModel.h"
#include "vtkCustomTransformInterpolator.h"
#include "vtkEigenTools.h"

#include "lvFiltersCameraModule.h"

class LVFILTERSCAMERA_EXPORT vtkCameraProjector : public vtkImageAlgorithm
{
public:
  static vtkCameraProjector *New();
  vtkTypeMacro(vtkCameraProjector, vtkImageAlgorithm)

  void SetFileName(const std::string &argfilename);

  vtkSetMacro(ProjectedPointSizeInImage, int);
  vtkSetMacro(UseTrajectoryToCorrectPoints, bool);
  vtkSetMacro(PipelineTimeToLidarTime, double);
  vtkSetMacro(ColorizedOutputOnly, double);

  void SetUseCalibrationFile(bool argUseCalibrationFile);

  // Camera model parameters
  void SetCameraType(int type);
  void SetRotation(double roll, double pitch, double yaw);
  void SetTranslation(double x, double y, double z);
  void SetFocal(double fx, double fy);
  void SetOpticalCenter(double cx, double cy);
  void SetSkew(double skew);
  void SetKCoeffs(double k1, double k2);
  void SetPCoeffs(double p1, double p2, double p3 = 0., double p4 = 0.);
  void SetCameraModelParams();

protected:
  vtkCameraProjector();

  int FillInputPortInformation(int port, vtkInformation *info) override;
  int FillOutputPortInformation(int port, vtkInformation *info) override;
  int RequestInformation(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  void Modified() override;

private:
  vtkCameraProjector(const vtkCameraProjector&) = delete;
  void operator=(const vtkCameraProjector&) = delete;

  //! Camera model
  CameraModel Model;
  bool ModelValid = false;
  double CurrentImagePipelineTime = 0.0; // time of the image given to RequestData

  vtkSmartPointer<vtkCustomTransformInterpolator> Trajectory = nullptr;


  //! File containing the camera model and parameters
  std::string Filename;

  //! Name of the color array
  std::string ColorArrayName = "Color";

  //! Size of the points in image with projection
  int ProjectedPointSizeInImage = 1;

  //! Should the cache be refreshed before next projection ?
  bool NeedsToUpdateCachedValues = false;

  //! Should the trajectory (if available) be used to correct the point
  //! coordinates ?
  //! - take into account the fact that the lidar frame sampling is different
  //! than the camera frame sampling
  //! - undistortion (lidar frame capturing is not instantaneous)
  bool UseTrajectoryToCorrectPoints = true;

  //! Timeshift to apply to the pipeline time to get the lidar time.
  //! This value could be computed on the current frame, but it much better to
  //! compute an estimator such as the median using all lidar frames
  //! used only if a trajectory is provided and UseTrajectoryToCorrectPoints is
  //! true
  double PipelineTimeToLidarTime = 0.0;

  double ColorizedOutputOnly = false;

  //! Should the calibration file be used ?
  bool UseCalibrationFile = true;

    //! intrinsic parameters of the camera model
  Eigen::Matrix3d K = Eigen::Matrix3d::Identity();

  //! extrinsic parameters of the camera model
  Eigen::Matrix3d R = Eigen::Matrix3d::Identity();
  Eigen::Vector3d T = Eigen::Vector3d::Zero();

  //! optical parameters of the camera
  Eigen::VectorXd Optics = Eigen::VectorXd::Zero(6);

  //! type of camera model
  ProjectionType Type = ProjectionType::Pinhole;
};

#endif // VTK_CAMERA_PROJECTOR_H
