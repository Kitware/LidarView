/*=========================================================================

  Program:   LidarView
  Module:    KalmanTracker.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef KALMAN_TRACKER_H
#define KALMAN_TRACKER_H

#include <Eigen/Dense>
#include <vector>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include "lvFiltersTemporalModule.h"

#include "vtkLVBBoxDetection.h"

using Vec3 = Eigen::Vector3d;
using Mat3 = Eigen::Matrix3d;
using SharedBBoxDetection = std::shared_ptr<const vtkLVBBoxDetection>;

/**
 * This class represent a Tracker that keep tracks of a Detection
 * through time using a KalmanFilter.
 */
class LVFILTERSTEMPORAL_EXPORT KalmanTracker
{
public:
  KalmanTracker(int id,
    SharedBBoxDetection detection,
    double p,
    double q,
    double rpos[3],
    double rsize[3],
    double rtheta);

  /**
   * Create transition matrix from a time delta
   */
  static Eigen::Matrix<double, 10, 10> makeF(double dt);

  /**
   * Merge a vector of KalmanTrackers and return the index of the tracker that 'absorbed' others
   */
  static size_t MergeTrackers(std::vector<std::shared_ptr<KalmanTracker>> trackers);

  /**
   * Predict the next state
   */
  void Predict(double dt);

  /**
   * Update state using a measurement
   */
  void Update(SharedBBoxDetection det);

  /**
   * Reset the state tracker using a new detection
   */
  void Reset(SharedBBoxDetection det);

  /**
   * Compute the distance between this tracker and a vtkLVBBoxDetection
   */
  double DistanceTo(const vtkLVBBoxDetection* other,
    double iouCoef,
    double positionCoef,
    double sizeCoef,
    double thetaCoef) const;

  /**
   * Return the Id of the Tracker
   */
  int GetId() const { return this->id; }

  /**
   * Return the 'Age' of the Tracker
   * (Number of frames since creation)
   */
  int GetAge() const { return this->age; }

  /**
   * Set Tracker Age to a custom value
   */
  void SetAge(int age) { this->age = age; }

  /**
   * Increment Age by 1
   */
  int IncrementAge() { return ++this->age; }

  /**
   * Return the color associated to this Tracker
   */
  int GetColor() const { return this->color; }

  /**
   * Return the number of frames since the Tracker didn't match any vtkLVBBoxDetection
   */
  int GetLostCounter() const { return this->lostCounter; }

  /**
   * Set LostCounter to a custom value
   */
  void SetLostCounter(int n) { this->lostCounter = n; }

  /**
   * Increment LostCounter by 1
   */
  int IncrementLostCounter() { return ++this->lostCounter; }

  /**
   * Return the position of the Tracker
   */
  Vec3 GetPos() const { return x.head<3>(); }

  /**
   * Return the velocity of the Tracker
   */
  Vec3 GetVelocity() const { return x.segment<3>(3); }

  /**
   * Return the size of the Tracker
   */
  Vec3 GetSize() const { return x.segment<3>(6); }

  /**
   * Return the orientation arround z-axis of the Tracker (radians)
   */
  double GetTheta() const { return x(9); }

  /**
   * Return a copy of the kalman filter vector state
   */
  Eigen::VectorXd getState() const { return x; }

  /**
   * Return the 'confidence' of the Track (between 0-10)
   */
  double GetConfidence() const { return confidence; }

  /**
   * Set the 'confidence' of the Track (capped between 0-10)
   */
  void SetConfidence(double value) { confidence = std::clamp(value, 0.0, 10.0); }

  /**
   * Increment 'confidence' by 'delta'
   */
  void UpdateConfidence(double delta) { SetConfidence(confidence + delta); }

  /**
   * Get the associated detection to this Tracker
   */
  SharedBBoxDetection GetDetection() const { return this->detection; }

  /**
   * Change the associated detection to this Tracker
   */
  void SetDetection(SharedBBoxDetection detection) { this->detection = detection; }

  /**
   * Create a new Detection from Tracker state
   */
  vtkLVBBoxDetection ToDetection() const
  {
    return vtkLVBBoxDetection(
      -1, this->GetPos(), this->GetSize(), this->GetTheta(), this->detection->GetFieldData());
  }

  /**
   * Create a cuboid PolyData from current state and associated detection
   */
  vtkSmartPointer<vtkPolyData> GetBoundingBoxPolyData() const;

private:
  int id;
  int color;
  int age = 0;
  int lostCounter = 0;
  double confidence = 3.0;
  double rot = 0;
  double p;
  SharedBBoxDetection detection;

  // Transition matrix of KalmanFilter
  Eigen::Matrix<double, 10, 10> F;

  // KalmanFilter observation model
  Eigen::Matrix<double, 7, 10> H;

  // KalmanFilter state
  Eigen::Vector<double, 10> x;

  // KalmanFilter predicated estimate covariance
  Eigen::Matrix<double, 10, 10> P;

  // Covariance of the KalmanFilter measure noise
  Eigen::Matrix<double, 7, 7> R;

  // Covariance of the KalmanFilter process noise
  Eigen::Matrix<double, 10, 10> Q;
};

using SharedTracker = std::shared_ptr<KalmanTracker>;

#endif