/*=========================================================================

  Program:   LidarView
  Module:    CameraModel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef CAMERA_MODEL_H
#define CAMERA_MODEL_H

// STD
#include <string>
#include <vector>

// EIGEN
#include <Eigen/Dense>

#include "lvCommonYamlModule.h"

enum ProjectionType
{
  Pinhole = 0,
  BrownConradyPinhole = 1,
  FishEye = 2
};

/**
 * @brief CameraModel class that represents a camera model included
 *        by the following modelization:
 *        1- Extrinsic parameters, element of SE(3)
 *        2- Intrinsic parameters, representing the focal and pixel grid
 *        3- Optical parameters, representing the optical system distortions
 */
class LVCOMMONYAML_EXPORT CameraModel
{
public:
  CameraModel() = default;

  // Setters
  void SetParams(Eigen::VectorXd argW);
  void SetK(Eigen::Matrix3d argK);
  void SetR(Eigen::Matrix3d argR);
  void SetT(Eigen::Vector3d argT);
  void SetOptics(Eigen::VectorXd argOptics);
  void SetCameraModelType(ProjectionType type);

  // Getters
  Eigen::VectorXd GetParametersVector();
  Eigen::Matrix3d GetK();
  Eigen::Matrix3d GetR();
  Eigen::Vector3d GetT();
  Eigen::VectorXd GetOptics();
  ProjectionType GetType();

  // Returns true when parameters have been read correctly
  bool LoadParamsFromFile(std::string filename);

  static void WriteParamsToFile(std::string outFilename,
    Eigen::VectorXd Win,
    ProjectionType typein);

  Eigen::Vector2d Projection(const Eigen::Vector3d& X, bool shouldClip = false);

protected:
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

#endif // CAMERA_MODEL_H
