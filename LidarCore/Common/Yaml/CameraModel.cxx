/*=========================================================================

  Program:   LidarView
  Module:    CameraModel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "CameraModel.h"
#include "CameraProjection.h"

// LOCAL
#include "vtkEigenTools.h"

#include <vtkLogger.h>

// YAML
#include <yaml-cpp/yaml.h>

//----------------------------------------------------------------------------
void CameraModel::SetParams(Eigen::VectorXd argW)
{
  // The parameters should at least contains
  // the 11 parameters of the pinhole model
  if (argW.size() < 11)
  {
    return;
  }

  this->R = RollPitchYawToMatrix(argW(0), argW(1), argW(2));
  this->T = Eigen::Vector3d(argW(3), argW(4), argW(5));
  this->K = Eigen::Matrix3d::Zero();
  this->K(0, 0) = argW(6);
  this->K(1, 1) = argW(7);
  this->K(0, 2) = argW(8);
  this->K(1, 2) = argW(9);
  this->K(0, 1) = argW(10);
  this->K(2, 2) = 1;

  this->Optics = Eigen::VectorXd::Zero(argW.size() - 11);
  for (int i = 0; i < argW.size() - 11; ++i)
  {
    this->Optics(i) = argW(11 + i);
  }
}

//----------------------------------------------------------------------------
void CameraModel::SetK(Eigen::Matrix3d argK)
{
  this->K = argK;
}

//----------------------------------------------------------------------------
void CameraModel::SetR(Eigen::Matrix3d argR)
{
  this->R = argR;
}

//----------------------------------------------------------------------------
void CameraModel::SetT(Eigen::Vector3d argT)
{
  this->T = argT;
}

//----------------------------------------------------------------------------
void CameraModel::SetOptics(Eigen::VectorXd argOptics)
{
  this->Optics = argOptics;
}

//----------------------------------------------------------------------------
void CameraModel::SetCameraModelType(ProjectionType type)
{
  this->Type = type;
}

//----------------------------------------------------------------------------
Eigen::Matrix3d CameraModel::GetK()
{
  return this->K;
}

//----------------------------------------------------------------------------
Eigen::Matrix3d CameraModel::GetR()
{
  return this->R;
}

//----------------------------------------------------------------------------
Eigen::Vector3d CameraModel::GetT()
{
  return this->T;
}

//----------------------------------------------------------------------------
Eigen::VectorXd CameraModel::GetOptics()
{
  return this->Optics;
}

//----------------------------------------------------------------------------
ProjectionType CameraModel::GetType()
{
  return this->Type;
}

//----------------------------------------------------------------------------
Eigen::VectorXd CameraModel::GetParametersVector()
{
  Eigen::VectorXd W;

  switch (this->Type)
  {
    case ProjectionType::Pinhole:
      W = Eigen::VectorXd(11);
      break;
    case ProjectionType::BrownConradyPinhole:
      W = Eigen::VectorXd(17);
      break;
    case ProjectionType::FishEye:
      W = Eigen::VectorXd(15);
      break;
  }

  W.block(0, 0, 3, 1) = MatrixToRollPitchYaw(this->R);
  W.block(3, 0, 3, 1) = this->T;
  W(6) = this->K(0, 0);
  W(7) = this->K(1, 1);
  W(8) = this->K(0, 2);
  W(9) = this->K(1, 2);
  W(10) = this->K(0, 1);

  if (this->Type == ProjectionType::BrownConradyPinhole)
  {
    for (int i = 0; i < 6; ++i)
    {
      W(11 + i) = this->Optics(i);
    }
  }
  else if (this->Type == ProjectionType::FishEye)
  {
    for (int i = 0; i < 4; ++i)
    {
      W(11 + i) = this->Optics(i);
    }
  }

  return W;
}

//------------------------------------------------------------------------------
bool CameraModel::LoadParamsFromFile(std::string filename)
{
  Eigen::VectorXd W;
  YAML::Node calib;
  std::string type;

  // Load calibration file
  try
  {
    calib = YAML::LoadFile(filename);
    type = calib["calib"]["type"].as<std::string>();
  }
  catch (const std::exception& e)
  {
    // Calibration file not valid
    return false;
  }

  if (type == "Pinhole")
  {
    W = Eigen::VectorXd(11);
    this->Type = ProjectionType::Pinhole;
  }
  else if (type == "FishEye")
  {
    W = Eigen::VectorXd(15);
    this->Type = ProjectionType::FishEye;
    // Set distortion parameters
    for (int i = 0; i < 2; ++i)
    {
      W(i + 11) = calib["calib"]["distortion"]["k_coeff"][i].as<double>();
    }
    for (int i = 0; i < 2; ++i)
    {
      W(i + 13) = calib["calib"]["distortion"]["p_coeff"][i].as<double>();
    }
  }
  else if (type == "BrownConradyPinhole")
  {
    W = Eigen::VectorXd(17);
    this->Type = ProjectionType::BrownConradyPinhole;
    // Set distortion parameters
    for (int i = 0; i < 2; ++i)
    {
      W(i + 11) = calib["calib"]["distortion"]["k_coeff"][i].as<double>();
    }
    for (int i = 0; i < 4; ++i)
    {
      W(i + 13) = calib["calib"]["distortion"]["p_coeff"][i].as<double>();
    }
  }
  else
  {
    return false;
  }

  // Load extrinsic parameters
  for (int i = 0; i < 3; ++i)
  {
    W(i) = calib["calib"]["extrinsic"]["rotation"][i].as<double>();
    W(i + 3) = calib["calib"]["extrinsic"]["translation"][i].as<double>();
  }

  // Load intrinsic parameters
  for (int i = 0; i < 2; ++i)
  {
    W(i + 6) = calib["calib"]["intrinsic"]["focal"][i].as<double>();
    W(i + 8) = calib["calib"]["intrinsic"]["optical_center"][i].as<double>();
  }
  W(10) = calib["calib"]["intrinsic"]["skew"].as<double>();

  this->SetParams(W);

  return true;
}

//------------------------------------------------------------------------------
void CameraModel::WriteParamsToFile(std::string outFilename,
  Eigen::VectorXd Win,
  ProjectionType typein)
{
  YAML::Node calib;

  // create nodes
  calib["calib"] = YAML::Node();

  switch (typein)
  {
    case ProjectionType::Pinhole:
      calib["calib"]["type"] = "Pinhole";
      break;
    case ProjectionType::BrownConradyPinhole:
      calib["calib"]["type"] = "BrownConradyPinhole";
      break;
    case ProjectionType::FishEye:
      calib["calib"]["type"] = "FishEye";
      break;
  }

  calib["calib"]["extrinsic"] = YAML::Node();
  calib["calib"]["extrinsic"]["rotation"] = YAML::Node();
  calib["calib"]["extrinsic"]["translation"] = YAML::Node();

  calib["calib"]["intrinsic"] = YAML::Node();
  calib["calib"]["intrinsic"]["focal"] = YAML::Node();
  calib["calib"]["intrinsic"]["optical_center"] = YAML::Node();
  calib["calib"]["intrinsic"]["skew"] = Win(10);

  calib["calib"]["distortion"] = YAML::Node();
  calib["calib"]["distortion"]["k_coeff"] = YAML::Node();
  calib["calib"]["distortion"]["p_coeff"] = YAML::Node();

  // populate them
  // Load extrinsic parameters
  for (int i = 0; i < 3; ++i)
  {
    calib["calib"]["extrinsic"]["rotation"].push_back(Win(i));
    calib["calib"]["extrinsic"]["translation"][i] = Win(i + 3);
  }

  // Load intrinsic parameters
  for (int i = 0; i < 2; ++i)
  {
    calib["calib"]["intrinsic"]["focal"][i] = Win(i + 6);
    calib["calib"]["intrinsic"]["optical_center"][i] = Win(i + 8);
    calib["calib"]["distortion"]["k_coeff"][i] = Win(i + 11);
  }
  for (int i = 0; i < 4; ++i)
  {
    calib["calib"]["distortion"]["p_coeff"][i] = Win(i + 13);
  }

  std::ofstream fout(outFilename.c_str());
  fout << calib;
}

//------------------------------------------------------------------------------
Eigen::Vector2d CameraModel::Projection(const Eigen::Vector3d& X, bool shouldClip)
{
  Eigen::VectorXd W = this->GetParametersVector();
  Eigen::Vector2d y(0, 0);
  switch (this->Type)
  {
    case ProjectionType::Pinhole:
      y = PinholeProjection(W, X, shouldClip);
      break;
    case ProjectionType::BrownConradyPinhole:
      y = BrownConradyPinholeProjection(W, X, shouldClip);
      break;
    case ProjectionType::FishEye:
      y = FisheyeProjection(W, X, shouldClip);
      break;
    default:
      vtkLog(WARNING, << "Warning: Projection type " << this->Type << " not handled!");
  }
  return y;
}
