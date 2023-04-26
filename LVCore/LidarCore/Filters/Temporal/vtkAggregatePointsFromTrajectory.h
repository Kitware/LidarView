/*=========================================================================

  Program:   LidarView
  Module:    vtkAggregatePointsFromTrajectory.h

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
#include "vtkVoxelGridProcessor.h"

// STL includes
#include <queue>
#include <string>

// VTK includes
#include <vtkCustomTransformInterpolator.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersTemporalModule.h"

/**
 * @brief The vtkAggregatePointsFromTrajectory class is a filter that aggregate points from temporal
 * point clouds using a trajectory. A voxel grid is used to filter the points.
 */
class LVFILTERSTEMPORAL_EXPORT vtkAggregatePointsFromTrajectory : public vtkPolyDataAlgorithm
{
public:
  static vtkAggregatePointsFromTrajectory* New();
  vtkTypeMacro(vtkAggregatePointsFromTrajectory, vtkPolyDataAlgorithm)

  vtkGetMacro(FirstFrame, int);
  vtkSetMacro(FirstFrame, int);

  vtkGetMacro(LastFrame, int);
  vtkSetMacro(LastFrame, int);

  vtkGetMacro(StepSize, int);
  vtkSetMacro(StepSize, int);

  vtkGetMacro(AllFrames, bool);
  vtkSetMacro(AllFrames, bool);

  int GetVoxelSamplingMode() { return VoxelGrid->GetSampling(); }
  void SetVoxelSamplingMode(int mode);

  int GetVoxelLeafSize() { return VoxelGrid->GetLeafSize(); }
  void SetVoxelLeafSize(double size);

  vtkGetVector6Macro(Bounds, double);
  vtkSetVector6Macro(Bounds, double);

  vtkGetMacro(TimeArrayName, std::string);
  vtkSetMacro(TimeArrayName, std::string);

  vtkGetMacro(ConversionFactorToSecond, double);
  vtkSetMacro(ConversionFactorToSecond, double);

  vtkGetMacro(TimeOffset, double);
  vtkSetMacro(TimeOffset, double);

  vtkGetMacro(InterpolationType, int);
  vtkSetMacro(InterpolationType, int);

protected:
  vtkAggregatePointsFromTrajectory();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  //! Overwrite FirstFrame and LastFrame to process all the frame
  bool AllFrames = true;

  //! First frame to be process
  int FirstFrame = 0;

  //! No frame after this frame will be processed. In case StepSize is not 1 it corresponds to
  //! the last frame processed
  int LastFrame = 10;

  //! Process one frame every StepSize frames (ex: every frame, every 2 frames, 3 frames, ...)
  int StepSize = 1;

  //! Name of the array containing the time to match the trajectory with the point cloud
  std::string TimeArrayName = "adjustedtime";

  //! Bounds of the voxel grid
  double Bounds[6];

  //! Double to convert time in second,
  //! Default is for data in microsecond
  double ConversionFactorToSecond = 1e-6;

  //! Offset to add to the time of the point cloud to match the trajectory
  //! The unit must be consistent with ConversionFactorToSecond
  double TimeOffset = 0;

  //! Interpolator used to get the right transform
  vtkSmartPointer<vtkCustomTransformInterpolator> Interpolator;
  //! Interpolation type used to interpolate the transform between two frames
  int InterpolationType = vtkCustomTransformInterpolator::INTERPOLATION_TYPE_LINEAR;

private:
  vtkAggregatePointsFromTrajectory(const vtkAggregatePointsFromTrajectory&);
  void operator=(const vtkAggregatePointsFromTrajectory&);

  //! Current frame to be processed
  int CurrentFrame = 0;

  //! Voxel grid filter used to aggregate the points
  vtkNew<vtkVoxelGridProcessor> VoxelGrid;

  //! Specify if the filter has been initialized
  bool Initialized = false;

  //! Specify if the trajectory time is considered as continuous with an offset between two
  //! consecutive poses
  bool ContinuousTrajectory = false;
};

#endif // vtkAggregatePointsFromTrajectory_H
