/*=========================================================================

  Program:   LidarView
  Module:    CameraProjection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef CAMERA_PROJECTION_H
#define CAMERA_PROJECTION_H

// STD
#include <string>
#include <vector>

// EIGEN
#include <Eigen/Dense>

#include "lvCommonCoreModule.h"

/**
 * @brief LoadCameraParamsFromCSV Load parameters from a csv file
 *
 * @param filename filename that contains the parameters
 * @param W loaded parameters
 */
LVCOMMONCORE_EXPORT void LoadCameraParamsFromCSV(std::string filename, Eigen::VectorXd& W);

/**
 * @brief WriteCameraParamsCSV Write parameters into a csv file
 *
 * @param filename filename to write the camera parameters
 * @param W to write parameters
 */
LVCOMMONCORE_EXPORT void WriteCameraParamsCSV(std::string filename, Eigen::VectorXd& W);

/**
 * @brief PinholeProjection Project a 3D point using a pinhole camera model
 *        the projected 2D points will be expressed in pixel coordinates
 *
 * @param W pinhole camera model parameters
 * @param X 3D point to project
 * @param shouldClip Clip points that are behind the camera plane
 */
LVCOMMONCORE_EXPORT Eigen::Vector2d PinholeProjection(const Eigen::Matrix<double, 15, 1>& W,
  const Eigen::Vector3d& X,
  bool shouldClip = false);

/**
 * @brief FisheyeProjection Project a 3D point using a fisheye camera model
 *        the projected 2D points will be expressed in pixel coordinates
 *
 * @param W fisheye camera model parameters
 * @param X 3D point to project
 * @param shouldClip Clip points that are behind the camera plane
 */
LVCOMMONCORE_EXPORT Eigen::Vector2d FisheyeProjection(const Eigen::Matrix<double, 15, 1>& W,
  const Eigen::Vector3d& X,
  bool shouldClip = false);

/**
 * @brief BrownConradyPinholeProjection Project a 3D point using a pinhole
 *        camera model with Brown-Conrady camera distortion model.
 *        the projected 2D points will be expressed in pixel coordinates
 *
 * @param W pinhole Brown-Conrady camera model parameters
 * @param X 3D point to project
 * @param shouldPlaneClip Clip points that are behind the camera plane
 * @param shouldFoVClip Clip points that are not in the FoV of the camera
 *        this is usefull since high distortion parameters can introduce
 *        non injective behaviour between 3D direction and 2D pixels
 * @param fovAngle angle of the field of view
 */
LVCOMMONCORE_EXPORT Eigen::Vector2d BrownConradyPinholeProjection(
  const Eigen::Matrix<double, 17, 1>& W,
  const Eigen::Vector3d& X,
  bool shouldPlaneClip = false);

/**
 * @brief GetRGBColourFromScalar map a scalar value
 *        onto an RGB color map.
 *
 * @param v scalar value to map
 * @param vmin minimal value of the scalar range
 * @param vmax maximal value of the scalar range
 */
LVCOMMONCORE_EXPORT Eigen::Vector3d GetRGBColourFromScalar(double v, double vmin, double vmax);

#endif // CAMERA_PROJECTION_H
