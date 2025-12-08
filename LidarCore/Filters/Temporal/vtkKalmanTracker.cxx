/*=========================================================================

  Program: LidarView
  Module:  vtkKalmanTracker.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <Eigen/Dense>
#include <unordered_set>
#include <vector>

#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnsignedIntArray.h>

#include "HungarianAlgorithm.h"
#include "vtkKalmanTracker.h"

constexpr unsigned int BOUNDING_BOX_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int BOUNDING_BOX_OUTPUT_PORT = 0;
constexpr unsigned int OUTPUT_PORT_COUNT = 1;

constexpr unsigned int INITIAL_TRACKERS_VECTOR_SIZE = 1000;

vtkStandardNewMacro(vtkKalmanTracker);

//-----------------------------------------------------------------------------
vtkKalmanTracker::vtkKalmanTracker()
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
  this->Trackers = std::vector<SharedTracker>();
  this->Trackers.reserve(INITIAL_TRACKERS_VECTOR_SIZE);
}

//-----------------------------------------------------------------------------
vtkKalmanTracker::~vtkKalmanTracker() = default;

//-----------------------------------------------------------------------------
void vtkKalmanTracker::Reset()
{
  this->Trackers = std::vector<SharedTracker>();
  this->Trackers.reserve(INITIAL_TRACKERS_VECTOR_SIZE);
  this->NextTrackerId = 0;
  this->Modified();
}

//-----------------------------------------------------------------------------
namespace
{
template <typename ArrayType, typename ScalarType>
void AddFieldArray(vtkFieldData* fieldData,
  const char* name,
  int numComponents,
  const ScalarType* data)
{
  auto column = vtkSmartPointer<ArrayType>::New();
  column->SetName(name);
  column->SetNumberOfComponents(numComponents);
  column->InsertNextTypedTuple(data);
  fieldData->AddArray(column);
}
}

//-----------------------------------------------------------------------------
void vtkKalmanTracker::AddBlock(vtkSmartPointer<vtkMultiBlockDataSet> outBoxes,
  const KalmanTracker* tracker,
  int color)
{
  outBoxes->SetBlock(this->BlockCounter, tracker->GetBoundingBoxPolyData());
  auto fieldData = tracker->GetDetection()->GetFieldData();

  unsigned int c = color;
  unsigned int id = tracker->GetId();
  unsigned int age = tracker->GetAge();
  unsigned int lost = tracker->GetLostCounter();
  double conf = tracker->GetConfidence();

  AddFieldArray<vtkUnsignedIntArray>(fieldData, "BlockColor", 1, &c);
  AddFieldArray<vtkUnsignedIntArray>(fieldData, "TrackId", 1, &id);
  AddFieldArray<vtkUnsignedIntArray>(fieldData, "Age", 1, &age);
  AddFieldArray<vtkUnsignedIntArray>(fieldData, "LostCounter", 1, &lost);
  AddFieldArray<vtkDoubleArray>(fieldData, "Confidence", 1, &conf);

  AddFieldArray<vtkDoubleArray>(fieldData, "Center", 3, tracker->GetPos().data());
  AddFieldArray<vtkDoubleArray>(fieldData, "Size", 3, tracker->GetSize().data());
  AddFieldArray<vtkDoubleArray>(fieldData, "Velocity", 3, tracker->GetVelocity().data());

  double theta = tracker->GetTheta();
  double quat[4] = { std::cos(theta / 2), 0, 0, std::sin(theta / 2) };
  AddFieldArray<vtkDoubleArray>(fieldData, "Orientation", 4, quat);

  auto block = outBoxes->GetBlock(this->BlockCounter++);
  block->SetFieldData(fieldData);
}

//-----------------------------------------------------------------------------
vtkKalmanTracker::MatchingResult vtkKalmanTracker::MatchTrackersAndDetections(
  std::vector<SharedBBoxDetection>& detections)
{
  int Ndet = detections.size();
  int Ntrk = this->Trackers.size();

  Eigen::MatrixXd distanceMatrix = Eigen::MatrixXd::Zero(Ndet, Ntrk);
  double maxDist = 1.0;
  for (int i = 0; i < Ndet; ++i)
  {
    for (int j = 0; j < Ntrk; ++j)
    {
      double distance = this->Trackers[j]->DistanceTo(detections[i].get(),
        this->DistanceCoefficients[0],
        this->DistanceCoefficients[1],
        this->DistanceCoefficients[2],
        this->DistanceCoefficients[3]);

      if (distance <= this->DistanceThreshold && distance > maxDist)
      {
        maxDist = distance;
      }
    }
  }

  double penalty = (maxDist + this->DistanceThreshold) * std::max(Ndet, Ntrk) * 2;

  for (int i = 0; i < Ndet; ++i)
  {
    for (int j = 0; j < Ntrk; ++j)
    {
      double distance = this->Trackers[j]->DistanceTo(detections[i].get(),
        this->DistanceCoefficients[0],
        this->DistanceCoefficients[1],
        this->DistanceCoefficients[2],
        this->DistanceCoefficients[3]);

      distanceMatrix(i, j) = (distance > this->DistanceThreshold) ? penalty : distance;
    }
  }

  // Hungarian assignment
  std::vector<int> assignedDetIndices;
  std::vector<int> assignedTrkIndices;

  HungarianAlgorithm::Solve(distanceMatrix, assignedDetIndices, assignedTrkIndices);

  std::vector<bool> detMatched(Ndet, false);
  std::vector<bool> trkMatched(Ntrk, false);

  std::vector<std::pair<int, SharedBBoxDetection>> matches;

  for (size_t k = 0; k < assignedDetIndices.size(); ++k)
  {
    int d = assignedDetIndices[k];
    int t = assignedTrkIndices[k];

    double dist = distanceMatrix(d, t);

    if (dist <= this->DistanceThreshold)
    {
      matches.emplace_back(t, detections[d]);
      detMatched[d] = true;
      trkMatched[t] = true;
    }
  }

  std::vector<int> unmatchedTrackers;
  for (int i = 0; i < Ntrk; ++i)
  {
    if (!trkMatched[i])
    {
      unmatchedTrackers.emplace_back(i);
    }
  }

  std::vector<SharedBBoxDetection> unmatchedDetections;
  for (int i = 0; i < Ndet; ++i)
  {
    if (!detMatched[i])
    {
      unmatchedDetections.emplace_back(detections[i]);
    }
  }

  return MatchingResult(matches, unmatchedTrackers, unmatchedDetections);
}

//-----------------------------------------------------------------------------
std::vector<SharedBBoxDetection> vtkKalmanTracker::MergeNewDetections(
  const std::vector<SharedBBoxDetection>& detections)
{
  std::vector<SharedBBoxDetection> resultDetections = detections;

  const size_t initialSize = this->Trackers.size();

  for (size_t i = 0; i < initialSize; i++)
  {
    const auto tracker = this->Trackers[i];
    const auto trackerDet = std::make_shared<vtkLVBBoxDetection>(tracker->ToDetection());
    auto detSet = std::unordered_set<SharedBBoxDetection>();
    for (auto detection : detections)
    {
      if (trackerDet->IoS(*detection) > 0.50)
      {
        detSet.insert(detection);
      }
    }

    if (detSet.size() >= 2)
    {
      if (tracker->GetConfidence() > 3.0)
      {
        resultDetections.erase(std::remove_if(resultDetections.begin(),
                                 resultDetections.end(),
                                 [&detSet](SharedBBoxDetection d) { return detSet.count(d) > 0; }),
          resultDetections.end());

        // We currently keep the tracker associated detection in this case
        resultDetections.push_back(trackerDet);
        tracker->UpdateConfidence(-0.2);
      }
      else
      {
        for (auto it = detSet.begin(); it != detSet.end(); ++it)
        {
          if (it == detSet.begin())
          {
            tracker->Reset(*it);
            tracker->SetConfidence(3);
          }
          else
          {
            auto newTracker = std::make_shared<KalmanTracker>(
              KalmanTracker(NextTrackerId++, *it, P, Q, RPos, RSize, RTheta));
            this->Trackers.push_back(newTracker);
          }
        }
      }
    }
  }

  return resultDetections;
}

//-----------------------------------------------------------------------------
std::vector<SharedBBoxDetection> vtkKalmanTracker::SplitNewDetections(
  std::vector<SharedBBoxDetection>& detections)
{
  std::vector<SharedBBoxDetection> resultDetections = detections;

  std::unordered_set<SharedBBoxDetection> detectionsToRemove;
  std::unordered_set<int> trackersToRemove;
  std::vector<SharedBBoxDetection> detectionsToAdd;

  for (size_t i = 0; i < detections.size(); i++)
  {
    const auto detection = detections[i];
    auto trackVec = std::vector<SharedTracker>();
    double averageConfidence = 0.0;

    for (const auto& track : this->Trackers)
    {
      const auto trackerDet = track->ToDetection();
      if (trackerDet.IoS(*detection) > 0.50)
      {
        trackVec.push_back(track);
        averageConfidence += track->GetConfidence();
      }
    }
    averageConfidence = !trackVec.empty() ? averageConfidence / trackVec.size() : 0.0;

    if (trackVec.size() >= 2)
    {
      if (averageConfidence > 3)
      {
        detectionsToRemove.insert(detection);
        for (auto track : trackVec)
        {
          auto fieldData = vtkSmartPointer<vtkFieldData>::New();
          fieldData->DeepCopy(detection->GetFieldData());
          const auto predictedDet = std::make_shared<vtkLVBBoxDetection>(
            0, track->GetPos(), track->GetSize(), track->GetTheta(), fieldData);
          detectionsToAdd.push_back(predictedDet);
          track->UpdateConfidence(-0.2);
        }
      }
      else
      {
        size_t keptTrackerIndex = KalmanTracker::MergeTrackers(trackVec);
        for (size_t j = 0; j < trackVec.size(); j++)
        {
          if (j != keptTrackerIndex)
          {
            trackersToRemove.insert(trackVec[j]->GetId());
          }
        }
      }
    }
  }

  if (!trackersToRemove.empty())
  {
    this->Trackers.erase(
      std::remove_if(this->Trackers.begin(),
        this->Trackers.end(),
        [&trackersToRemove](SharedTracker t) { return trackersToRemove.count(t->GetId()) > 0; }),
      this->Trackers.end());
  }

  if (!detectionsToRemove.empty())
  {
    resultDetections.erase(
      std::remove_if(resultDetections.begin(),
        resultDetections.end(),
        [&detectionsToRemove](SharedBBoxDetection d) { return detectionsToRemove.count(d) > 0; }),
      resultDetections.end());
  }

  if (!detectionsToAdd.empty())
  {
    for (auto detection : detectionsToAdd)
    {
      resultDetections.push_back(detection);
    }
  }

  return resultDetections;
}

//-----------------------------------------------------------------------------
int vtkKalmanTracker::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  auto inputBoxes =
    vtkMultiBlockDataSet::GetData(inputVector[BOUNDING_BOX_INPUT_PORT]->GetInformationObject(0));
  auto outputBoxes = vtkMultiBlockDataSet::GetData(outputVector, BOUNDING_BOX_OUTPUT_PORT);

  this->BlockCounter = 0;

  auto informationVector = inputVector[BOUNDING_BOX_INPUT_PORT];
  auto informationObject = informationVector->GetInformationObject(0);
  auto timeSteps = informationObject->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  double currentTime = inputBoxes->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  int frameIndex = -1;
  int i = 0;
  while (true)
  {
    if (timeSteps[i] == currentTime)
    {
      frameIndex = i;
      break;
    }
    i++;
  }
  const double previousTime = frameIndex > 0
    ? timeSteps[frameIndex - 1]
    : currentTime - (timeSteps[frameIndex + 1] - currentTime);
  const double dt = currentTime - previousTime;

  auto detections = vtkLVBBoxDetection::FromMultiBlockDataSet(inputBoxes);

  if (this->MergeAndSplitTracks)
  {
    detections = MergeNewDetections(detections);
    detections = SplitNewDetections(detections);
  }

  auto matchesResults = MatchTrackersAndDetections(detections);

  // Matches
  for (auto& res : matchesResults.matches)
  {
    auto trackerId = res.first;
    auto detection = res.second;
    auto tracker = this->Trackers[trackerId];

    if (this->ShowTracksPredictions)
    {
      this->AddBlock(outputBoxes, tracker.get(), 0);
    }
    tracker->SetDetection(detection);
    tracker->Update(detection);
    this->AddBlock(outputBoxes, tracker.get(), tracker->GetColor());
    tracker->Predict(dt);
    tracker->SetLostCounter(0);
    tracker->UpdateConfidence(0.1);
  }

  // Lost Trackers
  auto trackerIdsToRemove = std::unordered_set<int>();
  for (auto trackerId : matchesResults.unmatchedTrackers)
  {
    auto tracker = this->Trackers[trackerId];
    if (this->ShowGhostTracks)
    {
      this->AddBlock(outputBoxes, tracker.get(), 0);
    }
    tracker->Predict(dt);
    tracker->IncrementLostCounter();
    tracker->UpdateConfidence(-0.1);
    if (tracker->GetLostCounter() > this->MaxLostCounter)
    {
      trackerIdsToRemove.insert(tracker->GetId());
    }
  }
  std::vector<SharedTracker> newTrackers;
  newTrackers.reserve(this->Trackers.size());
  for (size_t i = 0; i < this->Trackers.size(); ++i)
  {
    if (trackerIdsToRemove.count(this->Trackers[i]->GetId()) == 0)
    {
      newTrackers.emplace_back(std::move(this->Trackers[i]));
    }
  }
  this->Trackers = std::move(newTrackers);

  // New Detections
  for (auto detection : matchesResults.unmatchedDetections)
  {
    auto newTracker = std::make_shared<KalmanTracker>(
      KalmanTracker(NextTrackerId++, detection, P, Q, RPos, RSize, RTheta));
    this->AddBlock(outputBoxes, newTracker.get(), newTracker->GetColor());
    this->Trackers.push_back(newTracker);
  }

  return 1;
}