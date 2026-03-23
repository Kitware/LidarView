//=========================================================================
// Copyright 2018-2020 Kitware, Inc.
// Author: Guilbert Pierre (Kitware SAS)
//         Cadart Nicolas (Kitware SAS)
// Date: 03-27-2018
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

#ifndef CERES_COST_FUNCTIONS_H
#define CERES_COST_FUNCTIONS_H

// LOCAL
#include "CeresTools.h"
// CERES
#include <ceres/jet.h>
#include <ceres/rotation.h>
// EIGEN
#include <Eigen/Geometry>

namespace CostFunctions
{
//------------------------------------------------------------------------------
/**
 * \class MahalanobisDistanceAffineIsometryResidual
 * \brief Cost function to optimize the affine isometry transformation
 *        (rotation and translation) that minimizes the mahalanobis distance
 *        between a point X and its neighborhood encoded by the mean point C
 *        and the variance covariance matrix A
 *
 * It takes one 6D parameters block :
 *   - 3 first parameters to encode rotation with euler angles : rX, rY, rZ
 *   - 3 last parameters to encode translation : X, Y, Z
 */
struct MahalanobisDistanceAffineIsometryResidual
{
  MahalanobisDistanceAffineIsometryResidual(const Eigen::Matrix3d& argA,
                                            const Eigen::Vector3d& argC,
                                            const Eigen::Vector3d& argX,
                                            double argWeight)
    : A(argA)
    , C(argC)
    , X(argX)
    , Weight(argWeight)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Matrix3T = Eigen::Matrix<T, 3, 3>;
    using Vector3T = Eigen::Matrix<T, 3, 1>;

    // Get translation part
    Eigen::Map<const Vector3T> trans(&w[3]);

    // Get rotation part, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T rot = Matrix3T::Identity();
    static T lastRot[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot))
    {
      rot = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot);
    }

    // Compute Y = R(theta) * X + T - C
    const Vector3T Y = rot * X + trans - C;

    // Compute final residual value which is:
    //   Ht * A * H with H = R(theta)X + T
    const T squaredResidual = Weight * (Y.transpose() * A * Y)(0);

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Matrix3d A;
  const Eigen::Vector3d C;
  const Eigen::Vector3d X;
  const double Weight;
};


//------------------------------------------------------------------------------
/**
 * \class MahalanobisDistanceLinearDistortionResidual
 * \brief Cost function to optimize the affine isometry transformation H1
 *        (rotation R1 and translation T1) so that the linearly interpolated
 *        transform
 *          (R, T) = (R0^(1-t) * R1^t, (1 - t)T0 + tT1)
 *        applied to X acquired at time t minimizes the mahalanobis distance
 *        between X and its neighborhood encoded by the mean point C and the
 *        variance covariance matrix A.
 *
 * It takes one 6D parameters block :
 *   - 3 first parameters to encode rotation R1 with euler angles : rX, rY, rZ
 *   - 3 last parameters to encode translation T1 : X, Y, Z
 */
struct MahalanobisDistanceLinearDistortionResidual
{
  MahalanobisDistanceLinearDistortionResidual(const Eigen::Matrix3d& argA,
                                              const Eigen::Vector3d& argC,
                                              const Eigen::Vector3d& argX,
                                              const Eigen::Vector3d& argT0,
                                              const Eigen::Matrix3d& argR0,
                                              double argTime,
                                              double argWeight)
    : A(argA)
    , C(argC)
    , X(argX)
    , T0(argT0)
    , R0(argR0)
    , Weight(argWeight)
    , Time(argTime)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Vector3T = Eigen::Matrix<T, 3, 1>;
    using Matrix3T = Eigen::Matrix<T, 3, 3>;
    using Isometry3T = Eigen::Transform<T, 3, Eigen::Isometry>;

    // Convert internal double matrix to a Jet matrix for auto diff calculous
    Matrix3T R0j = R0.cast<T>();
    Vector3T T0j = T0.cast<T>();
  
    // Get translation T1 part
    Eigen::Map<const Vector3T> T1j(&w[3]);

    // Get rotation part R1, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T R1j = Matrix3T::Identity();
    static T lastRot[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot))
    {
      R1j = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot);
    }

    // Compute the transform to apply to X depending on (R0, T0) and (R1, T1).
    // The applied isometry will be the linear interpolation between them :
    // (R, T) = (R0^(1-t) * R1^t, (1 - t)T0 + tT1)
    const Isometry3T H = ceres::LinearInterpolation<T>(R0j, T0j, R1j, T1j, Time);

    // Compute final residual value which is:
    // Yt * A * Y with Y = R(theta) * X + T - C
    const Vector3T Y = H.linear() * X + H.translation()  - C;
    const T squaredResidual = Weight * (Y.transpose() * A * Y)(0);

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Matrix3d A;
  const Eigen::Vector3d C;
  const Eigen::Vector3d X;
  const Eigen::Vector3d T0;
  const Eigen::Matrix3d R0;
  const double Weight;
  const double Time;
};


//------------------------------------------------------------------------------
/**
 * \class MahalanobisDistanceIsometryAndLinearDistortionResidual
 * \brief Cost function to optimize the rotation R1 and translation T1 so that
 *        the linearly interpolated transform:
 *          (R, T) = (R1 * R1^t, t * R1 * T1 + T1)
 *        applies to X acquired at time t minimizes the mahalanobis distance.
 *
 * It takes one 6D parameters block :
 *   - 3 first parameters to encode rotation R1 with euler angles : rX, rY, rZ
 *   - 3 last parameters to encode translation T1 : X, Y, Z
 */
struct MahalanobisDistanceIsometryAndLinearDistortionResidual
{
  MahalanobisDistanceIsometryAndLinearDistortionResidual(const Eigen::Matrix3d& argA,
                                                         const Eigen::Vector3d& argC,
                                                         const Eigen::Vector3d& argX,
                                                         double argTime,
                                                         double argWeight)
    : A(argA)
    , C(argC)
    , X(argX)
    , Time(argTime)
    , Weight(argWeight)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Vector3T = Eigen::Matrix<T, 3, 1>;
    using Matrix3T = Eigen::Matrix<T, 3, 3>;
    using Isometry3T = Eigen::Transform<T, 3, Eigen::Isometry>;

    // Get translation T1 part
    Eigen::Map<const Vector3T> T1(&w[3]);

    // Get rotation part R1, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T R1 = Matrix3T::Identity();
    static T lastRot[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot))
    {
      R1 = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot);
    }

    // Compute the transform to apply to X depending on (R0, T0) and (R1, T1).
    // The applied isometry will be the linear interpolation between them :
    // (R, T) = (R0^(1-t) * R1^t, (1 - t)T0 + tT1)
    const Isometry3T H = ceres::LinearInterpolation<T>(R1, T1, R1 * R1, R1 * T1 + T1, Time);

    // Compute final residual value which is:
    //  Yt * A * Y with Y = R(theta) * X + T - C
    const Vector3T Y = H.linear() * X + H.translation() - C;
    const T squaredResidual = Weight * (Y.transpose() * A * Y)(0);

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Matrix3d A;
  const Eigen::Vector3d C;
  const Eigen::Vector3d X;
  const double Time;
  const double Weight;
};


//------------------------------------------------------------------------------
/**
 * \class MahalanobisDistanceInterpolatedMotionResidual
 * \brief Cost function to optimize the isometries (R0, T0) and  (R1, T1) so that:
 *        The linearly interpolated transform:
 *        (R, T) = (R0^(1-t) * R1^t, (1 - t)T0 + tT1)
 *        applies to X acquired at time t minimizes the mahalanobis distance.
 *
 * It takes one 12D parameters blocks :
 *   - 3 parameters (0, 1, 2) to encode rotation R0 with euler angles : rX, rY, rZ
 *   - 3 parameters (3, 4, 5) to encode translation T0 : X, Y, Z
 *   - 3 parameters (6, 7, 8) to encode rotation R1 with euler angles : rX, rY, rZ
 *   - 3 parameters (9, 10, 11) to encode translation T1 : X, Y, Z
 */
struct MahalanobisDistanceInterpolatedMotionResidual
{
  MahalanobisDistanceInterpolatedMotionResidual(const Eigen::Matrix3d& argA,
                                                const Eigen::Vector3d& argC,
                                                const Eigen::Vector3d& argX,
                                                double argTime,
                                                double argWeight)
    : A(argA)
    , C(argC)
    , X(argX)
    , Time(argTime)
    , Weight(argWeight)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Vector3T = Eigen::Matrix<T, 3, 1>;
    using Matrix3T = Eigen::Matrix<T, 3, 3>;
    using Isometry3T = Eigen::Transform<T, 3, Eigen::Isometry>;

    // Get translation parts
    Eigen::Map<const Vector3T> T0(&w[3]);
    Eigen::Map<const Vector3T> T1(&w[9]);

    // Get rotation parts, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T R0 = Matrix3T::Identity();
    static Matrix3T R1 = Matrix3T::Identity();
    static T lastRot0[3] = {T(-1.), T(-1.), T(-1.)};
    static T lastRot1[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot0))
    {
      R0 = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot0);
    }
    if (!std::equal(w + 6, w + 9, lastRot1))
    {
      R1 = ceres::RollPitchYawToMatrix(w[6], w[7], w[8]);
      std::copy(w + 6, w + 9, lastRot1);
    }

   // Compute the transform to apply to X depending on (R0, T0) and (R1, T1).
    // The applied isometry will be the linear interpolation between them :
    // (R, T) = (R0^(1-t) * R1^t, (1 - t)T0 + tT1)
    const Isometry3T H = ceres::LinearInterpolation<T>(R0, T0, R1, R1, Time);

    // Compute final residual value which is:
    //  Yt * A * Y with Y = R(theta) * X + T - C
    const Vector3T Y = H.linear() * X + H.translation() - C;
    const T squaredResidual = Weight * (Y.transpose() * A * Y)(0);

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Matrix3d A;
  const Eigen::Vector3d C;
  const Eigen::Vector3d X;
  const double Time;
  const double Weight;
};


//------------------------------------------------------------------------------
/**
 * \class FrobeniusDistanceRotationCalibrationResidual
 * \brief Cost function to optimize the calibration rotation between two sensors
 *        based on their trajectory. To do that, we exploit the "solid-system"
 *        constraint that links the coordinate reference frame of the two sensors.
 *
 * Using the geometric constraints that come from the solid-assumption, it is
 * possible to estimate the 3-DoF rotation calibration (R in SO(3)) between
 * two sensors by using the estimation of their poses over the time.
 *
 * Let's t0 and t1 be two times.
 * Let's R be the orientation of the sensor 1 according to the sensor 2 reference frame.
 * Since we have the solid-assumption, R is constant and not time depending.
 *
 * Let's P0 (resp P1) in SO(3) be the orientation of the sensor 1 at time t0 (resp t1)
 *
 * Let's Q0 (resp Q1) in SO(3) be the orientation of the sensor 2 at time t0 (resp t1)
 *
 * From these two temporal points on the poses "trajectory", it is possible to express
 * the change of orientation between the sensor 1 at time t0 and sensor 1 at time t1
 * using two differents way
 *
 * 1- By using the poses of the sensor 1 at time t0 and t1:
 *    dR0 = P0' * P1
 *
 * 2- By using the solid-assumption and firstly express the point in the
 *    other sensor reference frame using the calibration parameters. Then, using
 *    method 1 for the second sensor and finally using the calibration again to
 *    dR1 = R' * Q0' * Q1 * R
 *
 * And finally, we are looking for R that satisfy:
 * dR1 = dR0
 *
 * This lead to a non-linear least square problem that can be solved using a
 * levenberg-marquardt algorithm. The non-linearity comes from R belonging to
 * SO(3) manifold. We estimate R using the Euler-Angle mapping between R^3 and SO(3) :
 * R(rx, ry, rz) = Rz(rz) * Ry(ry) * Rx(rx)
 *
 * It takes one parameters block of 3 parameters to encode rotation R with euler
 * angles : rX, rY, rZ.
 */
struct FrobeniusDistanceRotationCalibrationResidual
{
  FrobeniusDistanceRotationCalibrationResidual(const Eigen::Matrix3d& argP0,
                                               const Eigen::Matrix3d& argP1,
                                               const Eigen::Matrix3d& argQ0,
                                               const Eigen::Matrix3d& argQ1)
    : P0(argP0)
    , P1(argP1)
    , Q0(argQ0)
    , Q1(argQ1)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Matrix3T = Eigen::Matrix<T, 3, 3>;

    // Get rotation matrix from euler angles, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T R = Matrix3T::Identity();
    static T lastRot[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot))
    {
      R = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot);
    }

    // Compute the residual matrix
    const Matrix3T residualMatrix = R.transpose() * Q0.transpose() * Q1 * R - P0.transpose() * P1;

    // Compute final residual value, which is the frobenius norm of the residual matrix
    const T squaredResidual = (residualMatrix.transpose() * residualMatrix).trace();

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Matrix3d P0, P1, Q0, Q1;
};


//------------------------------------------------------------------------------
/**
 * \class FrobeniusDistanceRotationAndTranslationCalibrationResidual
 * \brief Cost function to optimize the calibration rotation and translation
 *        between two sensors based on their trajectory. To do that, we exploit
 *        the "solid-system" constraint that links the coordinate reference
 *        frame of the two sensors.
 *
 * Using the geometric constraints that come from the solid-assumption, it is
 * possible to estimate the 6-DoF calibration (R in SO(3), T in R^3) between
 * two sensors by using the estimation of their poses over the time.
 *
 * Let's t0 and t1 be two times.
 * Let's R, T be the pose of the sensor 1 according to the sensor 2 reference frame.
 * Since we have the solid-assumption, R and T are constant and not time depending.
 *
 * Let's P0 (resp P1) in SO(3) be the orientation of the sensor 1 at time t0 (resp t1).
 * Let's V0 (resp V1) in R^3 be the position of the sensor 1 at time t0 (resp t1).
 *
 * Let's Q0 (resp Q1) in SO(3) be the orientation of the sensor 2 at time t0 (resp t1).
 * Let's U0 (resp U1) in R^3 be the position of the sensor 2 at time t0 (resp t1).
 *
 * From these two temporal points on the poses "trajectory", it is possible to
 * express the change of reference frame between the sensor 1 at time t0 and
 * sensor 1 at time t1 using two differents way :
 *
 * 1- By using the poses of the sensor 1 at time t0 and t1:
 *    dR0 = P0' * P1
 *    dT0 = P0' * (V1 - V0)
 *
 * 2- By using the solid-assumption and firstly express the point in the other
 *    sensor reference frame using the calibration parameters. Then, using
 *    method 1 for the second sensor and finally using the calibration again to:
 *    dR1 = R' * Q0' * Q1 * R
 *    dT1 = R' * (Q0' * (Q1 * T + (U1 - U0)) - T)
 *
 * And finally, we are looking for (R, T) that satisfies:
 * dR1 = dR0
 * dT1 = dT0
 *
 * This lead to a non-linear least square problem that can be solved using a
 * levenberg-marquardt algorithm. The non-linearity comes from R belonging to
 * SO(3) manifold. We estimate R using the Euler-Angle mapping between R^3 and SO(3)
 * R(rx, ry, rz) = Rz(rz) * Ry(ry) * Rx(rx)
 *
 * It takes one 6D parameters block :
 *   - 3 first parameters to encode rotation R with euler angles : rX, rY, rZ
 *   - 3 last parameters to encode translation T : X, Y, Z
 */
struct FrobeniusDistanceRotationAndTranslationCalibrationResidual
{
  FrobeniusDistanceRotationAndTranslationCalibrationResidual(const Eigen::Matrix3d& argP0, const Eigen::Matrix3d& argP1,
                                                             const Eigen::Matrix3d& argQ0, const Eigen::Matrix3d& argQ1,
                                                             const Eigen::Vector3d& argV0, const Eigen::Vector3d& argV1,
                                                             const Eigen::Vector3d& argU0, const Eigen::Vector3d& argU1)
    : P0(argP0), P1(argP1)
    , Q0(argQ0), Q1(argQ1)
    , V0(argV0), V1(argV1)
    , U0(argU0), U1(argU1)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Matrix3T = Eigen::Matrix<T, 3, 3>;
    using Vector3T = Eigen::Matrix<T, 3, 1>;

    // Get translation part
    Eigen::Map<const Vector3T> trans(&w[3]);

    // Get rotation part from euler angles, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T rot = Matrix3T::Identity();
    static T lastRot[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot))
    {
      rot = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot);
    }

    // CHECK : do we need to consider both conditions on R and T ?
    const Eigen::Vector3d dT0 = P0.transpose() * (V1 - V0);
    const Vector3T dT1 = rot.transpose() * (Q0.transpose() * (Q1 * trans + (U1 - U0)) - trans);
    const T squaredResidual = (dT0 - dT1).squaredNorm();

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Matrix3d P0, P1, Q0, Q1;
  const Eigen::Vector3d V0, V1, U0, U1;
};


//------------------------------------------------------------------------------
/**
 * \class EuclideanDistanceAffineIsometryResidual
 * \brief Cost function to optimize the rotation and translation between two
 *        sensors based on their trajectory. To do that we minimize the
 *        euclidean distance between matched points from the trajectory
 *        according to the rotational and translational parameters. It results
 *        an affine transform "best" fit the second trajectory upon the second.
 *
 * Let's t in R be the current time.
 * Let's X be the position of the sensor 1 at time t.
 * Met's Y be the position of the sensor 2 at time t.
 *
 * We want to estimate R in SO(3) and T in R^3 such that
 * Y = R*X + T
 *
 * This lead to a non-linear least square problem that can be solved using a
 * levenberg-marquardt algorithm. The non-linearity comes from R belonging to
 * SO(3) manifold. We estimate R using the Euler-Angle mapping between R^3 and SO(3)
 * R(rx, ry, rz) = Rz(rz) * Ry(ry) * Rx(rx)
 *
 * It takes one 6D parameters block :
 *   - 3 first parameters to encode rotation R with euler angles : rX, rY, rZ
 *   - 3 last parameters to encode translation T : X, Y, Z
 */
struct EuclideanDistanceAffineIsometryResidual
{
  EuclideanDistanceAffineIsometryResidual(const Eigen::Vector3d& argX, const Eigen::Vector3d& argY)
    : X(argX)
    , Y(argY)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Matrix3T = Eigen::Matrix<T, 3, 3>;
    using Vector3T = Eigen::Matrix<T, 3, 1>;

    // Get translation part
    Eigen::Map<const Vector3T> trans(&w[3]);

    // Get rotation part from euler angles, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T rot = Matrix3T::Identity();
    static T lastRot[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot))
    {
      rot = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot);
    }

    const Vector3T L = rot * X + trans - Y;
    const T squaredResidual = L.squaredNorm();

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Vector3d X, Y;
};


//------------------------------------------------------------------------------
/**
 * \class SimilitudeResidual
 * \brief Cost function to optimize the rotation, translation and scale
 *        that map a set of input points to output points.
 *
 * Let's X be an input point
 * Met's Y be an out put point
 *
 * We want to estimate R in SO(3), T in R^3 and S in R^3 such that
 * Y = diag(S) * (RX + T)
 *
 * This lead to a non-linear least square problem that can be solved using a
 * levenberg-marquardt algorithm. The non-linearity comes from R belonging to
 * SO(3) manifold. We estimate R using the Euler-Angle mapping between R^3 and SO(3)
 * R(rx, ry, rz) = Rz(rz) * Ry(ry) * Rx(rx)
 *
 * It takes one 9D parameters block :
 *   - 3 first parameters [0-2] to encode rotation R with euler angles : rX, rY, rZ
 *   - 3 middle parameters [3-5] to encode translation T : X, Y, Z
 *   - 3 last parameters [6-8] to encode scale S : sX, sY, sZ
 */
struct SimilitudeResidual
{
  SimilitudeResidual(const Eigen::Vector3d& argX, const Eigen::Vector3d& argY)
    : X(argX)
    , Y(argY)
  {}

  template <typename T>
  bool operator()(const T* const w, T* residual) const
  {
    using Matrix3T = Eigen::Matrix<T, 3, 3>;
    using Vector3T = Eigen::Matrix<T, 3, 1>;

    // Get translation part
    Eigen::Map<const Vector3T> trans(&w[3]);
    // Get scale part
    Eigen::Map<const Vector3T> scale(&w[6]);

    // Get rotation part from euler angles, in a static way.
    // The idea is that all residual functions will need to evaluate those
    // sin/cos so we only compute them once each time the parameters change.
    static Matrix3T rot = Matrix3T::Identity();
    static T lastRot[3] = {T(-1.), T(-1.), T(-1.)};
    if (!std::equal(w, w + 3, lastRot))
    {
      rot = ceres::RollPitchYawToMatrix(w[0], w[1], w[2]);
      std::copy(w, w + 3, lastRot);
    }

    const Vector3T L = scale.asDiagonal() * (rot * X + trans) - Y;
    const T squaredResidual = L.squaredNorm();

    // Since t -> sqrt(t) is not differentiable in 0, we check the value of the
    // distance infenitesimale part. If it is not finite, it means that the
    // first order derivative has been evaluated in 0
    residual[0] = squaredResidual < 1e-6 ? T(0) : ceres::sqrt(squaredResidual);

    return true;
  }

private:
  const Eigen::Vector3d X, Y;
};
}

#endif // CERES_COST_FUNCTIONS_H
