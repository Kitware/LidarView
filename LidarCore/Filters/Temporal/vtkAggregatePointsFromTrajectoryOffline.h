/*=========================================================================

  Program:   LidarView
  Module:    vtkAggregatePointsFromTrajectoryOffline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkAggregatePointsFromTrajectory_H
#define vtkAggregatePointsFromTrajectory_H

// Local includes
#include "vtkAggregatePointsFromTrajectoryOnline.h"
#include "vtkMergePointsToPolyDataHelper.h"
#include "vtkVoxelGridProcessor.h"

// STL includes
#include <queue>

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersTemporalModule.h"

class vtkAggregatePointsFromTrajectoryOnline;

/**
 * @brief The vtkAggregatePointsFromTrajectoryOffline class is a filter that aggregate points from
 * temporal point clouds using a trajectory. A voxel grid is used to filter the points. If auto
 * compute bounds is set to true, the bounds of the voxel grid are computed by transforming the
 * bounds of the input point cloud using the trajectory.
 */

class LVFILTERSTEMPORAL_EXPORT vtkAggregatePointsFromTrajectoryOffline
  : public vtkAggregatePointsFromTrajectoryOnline
{
public:
  static vtkAggregatePointsFromTrajectoryOffline* New();
  vtkTypeMacro(vtkAggregatePointsFromTrajectoryOffline, vtkAggregatePointsFromTrajectoryOnline)

  vtkGetMacro(FirstFrame, int);
  vtkSetMacro(FirstFrame, int);

  vtkGetMacro(LastFrame, int);
  vtkSetMacro(LastFrame, int);

  vtkGetMacro(StepSize, int);
  vtkSetMacro(StepSize, int);

  vtkGetMacro(AllFrames, bool);
  vtkSetMacro(AllFrames, bool);

protected:
  vtkAggregatePointsFromTrajectoryOffline();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * @brief UpdateAutoComputeBoundsProgress Update the state of the autoComputeBounds process
   */
  void UpdateAutoComputeBoundsProgress(vtkInformation*) override;

  /**
   * @brief AggregatePoints Aggregate the points of the input point cloud using the voxel grid
   */
  int AggregatePoints(vtkInformation*,
    vtkInformation*,
    vtkInformationVector*,
    vtkPolyData*,
    vtkDataArray*) override;

  //! Overwrite FirstFrame and LastFrame to process all the frame
  bool AllFrames = true;

  //! First frame to be process
  int FirstFrame = 0;

  //! No frame after this frame will be processed. In case StepSize is not 1 it corresponds to
  //! the last frame processed
  int LastFrame = 10;

  //! Process one frame every StepSize frames (ex: every frame, every 2 frames, 3 frames, ...)
  int StepSize = 1;

private:
  vtkAggregatePointsFromTrajectoryOffline(const vtkAggregatePointsFromTrajectoryOffline&);
  void operator=(const vtkAggregatePointsFromTrajectoryOffline&);
};

#endif // vtkAggregatePointsFromTrajectory_H
