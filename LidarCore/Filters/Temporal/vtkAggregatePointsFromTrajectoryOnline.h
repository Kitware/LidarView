/*=========================================================================

  Program:   LidarView
  Module:    vtkAggregatePointsFromTrajectoryOnline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkAggregatePointsFromTrajectoryOnline_H
#define vtkAggregatePointsFromTrajectoryOnline_H

// Local includes
#include "vtkMergePointsToPolyDataHelper.h"
#include "vtkVoxelGridProcessor.h"

// STD includes
#include <queue>
#include <string>

// VTK includes
#include <vtkCustomTransformInterpolator.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersTemporalModule.h"

/**
 * @brief The vtkAggregatePointsFromTrajectoryOnline class is a filter that aggregate points from
 * temporal point clouds using a trajectory by playing the time data with the timeline. This way,
 * the frames to aggregate can be chosen individually. A voxel grid is used to filter the points. If
 * auto compute bounds is set to true, the bounds of the voxel grid are computed by transforming the
 * bounds of the input point cloud using the trajectory.
 */
class LVFILTERSTEMPORAL_EXPORT vtkAggregatePointsFromTrajectoryOnline : public vtkPolyDataAlgorithm
{
public:
  static vtkAggregatePointsFromTrajectoryOnline* New();
  vtkTypeMacro(vtkAggregatePointsFromTrajectoryOnline, vtkPolyDataAlgorithm)

  using PointCloudMap = std::unordered_map<std::string, vtkSmartPointer<vtkPolyData>>;

  /**
   * @brief Clear the voxel gird
   */
  void Clear();

  /**
   * @brief FreeUnusedMemory Free the extra memory allocated to avoid reallocation
   */
  void FreeUnusedMemory();

  int GetVoxelSamplingMode() { return VoxelGrid->GetSampling(); }
  void SetVoxelSamplingMode(int mode);

  int GetVoxelLeafSize() { return VoxelGrid->GetLeafSize(); }
  void SetVoxelLeafSize(double size);

  vtkGetMacro(AutoComputeBounds, bool);
  vtkSetMacro(AutoComputeBounds, bool);

  vtkGetVector6Macro(CustomBounds, double);
  vtkSetVector6Macro(CustomBounds, double);

  vtkGetMacro(IsVoxelGridFilterUsed, bool);
  vtkSetMacro(IsVoxelGridFilterUsed, bool);

  vtkGetMacro(AutoDetectTimeArray, bool);
  vtkSetMacro(AutoDetectTimeArray, bool);

  vtkGetMacro(CustomTimeArrayName, std::string);
  vtkSetMacro(CustomTimeArrayName, std::string);

  vtkGetMacro(AutoDetectTimeUnitConversion, bool);
  vtkSetMacro(AutoDetectTimeUnitConversion, bool);

  vtkGetMacro(CustomConversionFactorToSecond, double);
  vtkSetMacro(CustomConversionFactorToSecond, double);

  vtkGetMacro(TimeOffset, double);
  vtkSetMacro(TimeOffset, double);

  vtkGetMacro(RelativeTimestamp, bool);
  vtkSetMacro(RelativeTimestamp, bool);

  vtkGetMacro(ReferenceTimeName, std::string);
  vtkSetMacro(ReferenceTimeName, std::string);

  vtkGetMacro(InterpolationType, int);
  vtkSetMacro(InterpolationType, int);

  vtkGetMacro(DisplayOutput, bool);
  vtkSetMacro(DisplayOutput, bool);

protected:
  vtkAggregatePointsFromTrajectoryOnline();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * @brief InitializeData Initialize the filter by setting the useful parameters. This method is
   * called once at the beginning of the filter.
   */
  void InitializeData(vtkPolyData*, vtkPolyData*);

  /**
   * @brief To remove the offset when the input trajectory has large coordinates. This is to avoid
   * some numerical issue.
   */
  void CheckAndRemoveOffsetFromTrajectory(vtkPolyData* localTrajectory);

  /**
   * @brief AutoComputeVoxelBounds Compute the bounds of the voxel grid by transforming the bounds
   * of the input point cloud using the trajectory and calculating the global bounds of the
   * transformed bounds. This method is called sequentially once per frame before the aggregation.
   */
  int AutoComputeVoxelBounds(vtkInformation* request,
    vtkInformation* inInfo,
    PointCloudMap& vecPointcloud);

  /**
   * @brief UpdateAutoComputeBoundsProgress Update the state of the autoComputeBounds process
   */
  virtual void UpdateAutoComputeBoundsProgress(vtkInformation*);

  /**
   * @brief AggregatePoints Aggregate the points of the input point cloud using the voxel grid
   * filter.
   */
  virtual int AggregatePoints(vtkInformation* request,
    vtkInformation* inInfo,
    vtkInformationVector* outputVector,
    PointCloudMap& vecPointcloud);

  /**
   * @brief TransformAndAddPoints Transform the points of the input point cloud using the trajectory
   * and add them to the voxel grid.
   */
  virtual int TransformAndAddPoints(PointCloudMap& vecPointcloud);

  /**
   * @brief DetectTimeArray Checks AutoDetectTimeArray to get the correct Time array name
   */
  std::string GetTimeArrayName(vtkPolyData*);

  /**
   * @brief DetectTimeArray Detect a time array in the point cloud by searching for the words "time"
   * or "Time" in name of arrays and return its name
   */
  std::string DetectTimeArray(vtkPolyData*);

  /**
   * @brief  Init the polydata pointcloud with either a composite dataset input or a
   * or a polydata input
   */
  bool InitPointCloud(vtkInformation* inInfo, PointCloudMap& vecPolydata);

  /**
   * @brief Compute the time unit conversion between the trajectory and the point cloud
   */
  double ComputeTimeUnitConversion(vtkDataArray*, vtkDataArray*);

  /**
   * @brief Get timestamp from field data of pointcloud polydata
   */
  double GetTimeFromFieldData(vtkPolyData* pointcloud);

  //! If true, the time array is automatically detected
  bool AutoDetectTimeArray = true;
  //! Name of the array containing the time to match the trajectory with the point cloud
  //! Used only if AutoDetectTimeArray is false
  std::string CustomTimeArrayName = "adjustedtime";
  //! If true, the time unit conversion is automatically detected
  bool AutoDetectTimeUnitConversion = true;
  //! Double to convert time in second (default is for data in microsecond)
  //! Used only if AutoDetectTimeUnitConversion is false
  double CustomConversionFactorToSecond = 1e-6;

  //! If true, the bounds will be automatically computed
  bool AutoComputeBounds = true;
  //! Custom bounds of the voxel grid
  double CustomBounds[6] = { 0, 0, 0, 0, 0, 0 };

  //! Offset to add to the time of the point cloud to match the trajectory
  //! The unit must be consistent with ConversionFactorToSecond
  double TimeOffset = 0;

  //! Boolean to indicated whether the timestamp of a point is relative to the frame time
  bool RelativeTimestamp = false;
  //! Timestamps array name in field data used when RelativeTimestamp is true, differ from
  //! CustomTimeArrayName
  std::string ReferenceTimeName = "Timestamp";

  //! Boolean to store whether or not an offset is removed from trajectory
  bool IsOffsetRemoved = false;
  //! Offset of the input trajectory
  double OffsetOrigin[3] = { 0., 0., 0. };

  //! If true, the voxel grid filter is used to aggregate the points
  //! If false, the points are aggregated without filtering
  bool IsVoxelGridFilterUsed = true;

  //! Interpolator used to get the right transform
  vtkSmartPointer<vtkCustomTransformInterpolator> Interpolator;
  //! Interpolation type used to interpolate the transform between two frames
  int InterpolationType = vtkCustomTransformInterpolator::INTERPOLATION_TYPE_LINEAR;

  //! Current frame to be processed
  int CurrentFrame = 0;

  //! Voxel grid filter used to aggregate the points
  vtkNew<vtkVoxelGridProcessor> VoxelGrid;
  vtkNew<vtkMergePointsToPolyDataHelper> MergePointsToPolyDataHelper;

  //! Specify if the bounds have been computed
  bool AreBoundsComputed = false;

  //! Number of points in the frames
  //! Used to allocate the exact memory of the output point cloud when IsVoxelGridFilterUsed is
  //! false
  int NumberOfPoints = 0;

  //! Save the number of points allocated with resizeData
  //! This attribute allows to use SetPoint to add new points while the number of points is lower
  //! than the number of points allocated
  int InitialNumberOfPoints = 0;
  // Default number of points allocated in the output point cloud when IsVoxelGridFilterUsed is true
  int DefaultNumberOfPoints = 100000;

  //! Bounds of the voxel grid
  std::vector<double> Bounds = { 0, 0, 0, 0, 0, 0 };
  //! Current time array name
  std::string TimeArrayName;
  //! Current conversion factor
  double ConversionFactorToSecond = 1e-6;

  //! Specify if the filter has been initialized
  bool Initialized = false;
  //! Specify if the voxel grid has been initialized
  bool IsVoxelGridFilterInitialized = false;

  //! Specify if the trajectory time is considered as continuous with an offset between two
  //! consecutive poses
  bool ContinuousTrajectory = false;

  //! Specify if the output should be displayed
  bool DisplayOutput = true;

  //! Store the current frame time for multilidar setup
  std::unordered_map<std::string, double> FrameTime;

private:
  vtkAggregatePointsFromTrajectoryOnline(const vtkAggregatePointsFromTrajectoryOnline&);
  void operator=(const vtkAggregatePointsFromTrajectoryOnline&);
};

#endif // vtkAggregatePointsFromTrajectoryOnline_H
