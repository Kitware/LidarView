/*=========================================================================

  Program: LidarView
  Module:  vtkKalmanTracker.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_KALMAN_TRACKER_H
#define VTK_KALMAN_TRACKER_H

#include <vector>
#include <vtkMultiBlockDataSetAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include "KalmanTracker.h"
#include "lvFiltersTemporalModule.h"
#include "vtkLVBBoxDetection.h"

using SharedBBoxDetection = std::shared_ptr<const vtkLVBBoxDetection>;
using SharedTracker = std::shared_ptr<KalmanTracker>;

/**
 * @brief The KalmanFilter class contains the whole flow to track detections accross time.
 * The process is the following:
 *  - Get current frame detections
 *  - Predict previous tracks new positions (using Kalman Filter)
 *  - Compute matrix distance between frames detections & tracks predictions.
 *  - Solve associations using Hungarian algorithm.
 * Input: A multiblock dataset of boxes
 * Output: A multiblock dataset of boxes (with additional fields such as TrackId)
 */
class LVFILTERSTEMPORAL_EXPORT vtkKalmanTracker : public vtkMultiBlockDataSetAlgorithm
{
public:
  // struct to map results of Hungarian algorithm to tracks & detections
  struct MatchingResult
  {
    std::vector<std::pair<int, SharedBBoxDetection>> matches;
    std::vector<int> unmatchedTrackers;
    std::vector<SharedBBoxDetection> unmatchedDetections;

    MatchingResult(const std::vector<std::pair<int, SharedBBoxDetection>>& m,
      const std::vector<int>& umt,
      const std::vector<SharedBBoxDetection>& umd)
      : matches(m)
      , unmatchedTrackers(umt)
      , unmatchedDetections(umd)
    {
    }
  };

  static vtkKalmanTracker* New();
  vtkTypeMacro(vtkKalmanTracker, vtkMultiBlockDataSetAlgorithm)

  // Getters / Setters
  // Debug option to visualize tracks not associated to detections, but still alive.
  vtkSetMacro(ShowGhostTracks, bool);
  vtkGetMacro(ShowGhostTracks, bool);

  // Debug option to visualize tracks predictions. Useful to tune parameters.
  vtkSetMacro(ShowTracksPredictions, bool);
  vtkGetMacro(ShowTracksPredictions, bool);

  // Maximum distance to which a tracker and a detection can be associated.
  vtkSetMacro(DistanceThreshold, double);
  vtkGetMacro(DistanceThreshold, double);

  // Coefficients to use for distance metric: A * IoU + B * Euclidan distance + C * Size distance +
  // D * Angle distance. (IoU is intersection over union of bounding boxes).
  vtkSetVector4Macro(DistanceCoefficients, double);
  vtkGetVector4Macro(DistanceCoefficients, double);

  // The number of frames a tracker can survive while not being associated to a detection.
  vtkSetMacro(MaxLostCounter, int);
  vtkGetMacro(MaxLostCounter, int);

  // Allow the filter to try merging and splitting some tracks / detections.
  // It can help with noisy data where multiple detections can be created for a single object.
  vtkSetMacro(MergeAndSplitTracks, bool);
  vtkGetMacro(MergeAndSplitTracks, bool);

  // Kalman parameters
  // KalmanFilter estimated error covariance.
  vtkSetMacro(P, double);
  vtkGetMacro(P, double);

  // KalmanFilter process measure noise error covariance.
  vtkSetMacro(Q, double);
  vtkGetMacro(Q, double);

  // KalmanFilter position measure noise error covariance.
  vtkSetVector3Macro(RPos, double);
  vtkGetVector3Macro(RPos, double);

  // KalmanFilter size measure noise error covariance.
  vtkSetVector3Macro(RSize, double);
  vtkGetVector3Macro(RSize, double);

  // KalmanFilter z-orientation measure noise error covariance.
  vtkSetMacro(RTheta, double);
  vtkGetMacro(RTheta, double);

  void Reset();

  // Add a tracker to the filter output (multiblock dataset).
  void AddBlock(vtkSmartPointer<vtkMultiBlockDataSet> outBoxes,
    const KalmanTracker* tracker,
    int color);

  // Associate a vector of detections to existing tracks.
  MatchingResult MatchTrackersAndDetections(std::vector<SharedBBoxDetection>& detections);

  // Merge / Split bounding boxes based on new detections.
  std::vector<SharedBBoxDetection> MergeNewDetections(
    const std::vector<SharedBBoxDetection>& detections);
  std::vector<SharedBBoxDetection> SplitNewDetections(std::vector<SharedBBoxDetection>& detections);

protected:
  // constructor / destructor
  vtkKalmanTracker();
  ~vtkKalmanTracker();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  // Vector containing existing trackers
  std::vector<SharedTracker> Trackers;

  // Debug option to visualize tracks not associated to detections, but still alive.
  bool ShowGhostTracks = false;

  // Debug option to visualize tracks predictions. Useful to tune parameters.
  bool ShowTracksPredictions = false;

  // Maximum distance to which a tracker and a detection can be associated.
  double DistanceThreshold = 2.0;

  // Coefficients used to compute distance: A * IoU + B * Euclidean distance + C * Size distance + D
  // * Angle distance.
  double DistanceCoefficients[4] = { 3.0, 1.0, 0.0, 0.0 };

  // The number of frames a tracker can survive while not being associated to a detection.
  int MaxLostCounter = 7;

  // Allow the filter to try merging and splitting some tracks / detections.
  bool MergeAndSplitTracks = false;

  // Kalman Parameters (refer to getters / setters documentation)

  // Error Covariance
  double P = 100.0;

  // Process Noise Covariance
  double Q = 0.5;

  // Measurement Noise Covariance (Position, Size and Z-Rotation)
  double RPos[3] = { 1.5, 1.5, 5.0 };
  double RSize[3] = { 4.0, 4.0, 10.0 };
  double RTheta = 0.0;

  // The Id of the next created track.
  int NextTrackerId = 0;

  // Internal property to keep track of blocks in filter output.
  int BlockCounter = 0;
};

#endif