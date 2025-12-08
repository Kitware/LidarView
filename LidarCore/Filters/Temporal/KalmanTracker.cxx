/*=========================================================================

  Program:   LidarView
  Module:    KalmanTracker.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "KalmanTracker.h"
//------------------------------------------------------------------------------
KalmanTracker::KalmanTracker(int id,
  SharedBBoxDetection detection,
  double p,
  double q,
  double rpos[3],
  double rsize[3],
  double rtheta)
  : id(id)
  , p(p)
{
  vtkSmartPointer<vtkFieldData> newFieldData = vtkSmartPointer<vtkFieldData>::New();
  newFieldData->DeepCopy(detection->GetFieldData());
  auto det = vtkLVBBoxDetection(*detection);
  det.SetFieldData(newFieldData);
  this->detection = std::make_shared<vtkLVBBoxDetection>(det);

  this->color = std::rand() % 70 + 30;
  this->F = makeF(0.1);

  this->H = Eigen::Matrix<double, 7, 10>::Zero();
  H(0, 0) = 1;
  H(1, 1) = 1;
  H(2, 2) = 1;
  H(3, 6) = 1;
  H(4, 7) = 1;
  H(5, 8) = 1;
  H(6, 9) = 1;

  x = Eigen::Vector<double, 10>::Zero();

  P = Eigen::Matrix<double, 10, 10>::Identity() * p;
  Q = Eigen::Matrix<double, 10, 10>::Identity() * q;
  R = Eigen::Matrix<double, 7, 7>::Zero();
  R(0, 0) = rpos[0];
  R(1, 1) = rpos[1];
  R(2, 2) = rpos[2];
  R(3, 3) = rsize[0];
  R(4, 4) = rsize[1];
  R(5, 5) = rsize[2];
  R(6, 6) = rtheta;

  for (int i = 0; i < 3; ++i)
  {
    x(i) = detection->GetPosition()[i];
    x(i + 6) = detection->GetSize()[i];
  }
  x(9) = detection->GetTheta();
}

//------------------------------------------------------------------------------
Eigen::Matrix<double, 10, 10> KalmanTracker::makeF(double dt)
{
  Eigen::Matrix<double, 10, 10> F = Eigen::Matrix<double, 10, 10>::Identity();
  F(0, 3) = dt;
  F(1, 4) = dt;
  F(2, 5) = dt;
  return F;
}

//------------------------------------------------------------------------------
size_t KalmanTracker::MergeTrackers(std::vector<SharedTracker> trackers)
{
  std::vector<vtkLVBBoxDetection> detections;
  size_t mostConfidentIndex = 0;
  double bestConfidence = trackers[0]->ToDetection().GetVolume();
  detections.push_back(trackers[0]->ToDetection());
  for (size_t i = 1; i < trackers.size(); i++)
  {
    auto tracker = trackers[i];
    detections.push_back(tracker->ToDetection());
    double confidence = tracker->ToDetection().GetVolume();
    if (confidence > bestConfidence)
    {
      bestConfidence = confidence;
      mostConfidentIndex = i;
    }
  }
  SharedBBoxDetection globalDet =
    std::make_shared<vtkLVBBoxDetection>(vtkLVBBoxDetection::MergeDetections(detections));
  auto mainTracker = trackers[mostConfidentIndex];
  mainTracker->confidence = 3.0;
  mainTracker->Reset(globalDet);

  return mostConfidentIndex;
}

//------------------------------------------------------------------------------
void KalmanTracker::Predict(double dt)
{
  F = makeF(dt);
  x = F * x;
  P = F * P * F.transpose() + Q;
}

//------------------------------------------------------------------------------
void KalmanTracker::Update(SharedBBoxDetection det)
{
  double oldTheta = this->GetTheta();
  double newTheta = det->GetTheta();

  if (newTheta - oldTheta < -vtkMath::Pi() / 2)
  {
    rot++;
  }
  else if (newTheta - oldTheta > vtkMath::Pi() / 2)
  {
    rot--;
  }
  double theta = newTheta + rot * vtkMath::Pi();

  Eigen::VectorXd z(H.rows());
  for (int i = 0; i < 3; ++i)
  {
    z(i) = det->GetPosition()[i];
    z(i + 3) = det->GetSize()[i];
  }
  z(6) = theta;

  Eigen::VectorXd y = z - H * x;
  Eigen::MatrixXd S = H * P * H.transpose() + R;
  Eigen::MatrixXd K = P * H.transpose() * S.inverse();

  x = x + K * y;
  P = (Eigen::MatrixXd::Identity(P.rows(), P.cols()) - K * H) * P;
  this->IncrementAge();
  this->detection = det;
}

//------------------------------------------------------------------------------
void KalmanTracker::Reset(SharedBBoxDetection det)
{
  this->detection = det;
  P = Eigen::Matrix<double, 10, 10>::Identity() * p;
  for (int i = 0; i < 3; ++i)
  {
    x(i) = det->GetPosition()[i];
    x(i + 3) = 0.0;
    x(i + 6) = det->GetSize()[i];
  }
}

//------------------------------------------------------------------------------
double KalmanTracker::DistanceTo(const vtkLVBBoxDetection* other,
  double iouCoef,
  double positionCoef,
  double sizeCoef,
  double thetaCoef) const
{
  return ToDetection().DistanceTo(*other, iouCoef, positionCoef, sizeCoef, thetaCoef);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> KalmanTracker::GetBoundingBoxPolyData() const
{
  return ToDetection().GetBoundingBoxPolyData();
}