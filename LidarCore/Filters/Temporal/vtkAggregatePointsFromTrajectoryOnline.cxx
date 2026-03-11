/*=========================================================================

  Program:   LidarView
  Module:    vtkAggregatePointsFromTrajectoryOnline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// VTK includes
#include <vtkAppendDataSets.h>
#include <vtkAppendPolyData.h>
#include <vtkBoundingBox.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTemporalTransforms.h>
#include <vtkTransform.h>

// Local includes
#include "vtkAggregatePointsFromTrajectoryOnline.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAggregatePointsFromTrajectoryOnline)

//----------------------------------------------------------------------------
vtkAggregatePointsFromTrajectoryOnline::vtkAggregatePointsFromTrajectoryOnline()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  // Reset the bounds with min / max values
  for (int i = 0; i < 3; i++)
  {
    this->Bounds[i * 2] = std::numeric_limits<double>::max();
    this->Bounds[i * 2 + 1] = std::numeric_limits<double>::lowest();
  }
}

//-----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::FillInputPortInformation(int port, vtkInformation* info)
{
  // Pointcloud data
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
    return 1;
  }
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->AutoComputeBounds && !this->AreBoundsComputed)
  {
    // Get the time and force it
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    double time =
      *(inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) + this->CurrentFrame);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
  }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkAggregatePointsFromTrajectoryOnline::InitPointCloud(vtkInformation* inInfo,
  PointCloudMap& vecPolydata)
{
  vecPolydata.clear();
  // Only one pointcloud as input
  vtkSmartPointer<vtkPolyData> pointcloud = vtkPolyData::GetData(inInfo);
  if (pointcloud)
  {
    if (!pointcloud->GetPointData()->GetArray(this->GetTimeArrayName(pointcloud).c_str()))
    {
      vtkErrorMacro("No TimeStamp array found.");
      return false;
    }
    vecPolydata["mainLidar"] = pointcloud;
    if (this->RelativeTimestamp)
    {
      this->FrameTime["mainLidar"] = this->GetTimeFromFieldData(pointcloud);
    }
    return true;
  }

  // More than one pointcloud as input
  vtkCompositeDataSet* cds = vtkCompositeDataSet::GetData(inInfo);
  if (!cds)
    return false;

  int inputId = 0;
  // Get input iterator
  vtkCompositeDataIterator* it = cds->NewIterator();
  it->InitTraversal();
  it->GoToFirstItem();
  while (!it->IsDoneWithTraversal())
  {
    vtkPolyData* current = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (!current)
    {
      vtkErrorMacro("CompositeDataSet isn't exclusively composed of polydata");
      return false;
    }
    // Get device id
    vtkInformation* info = it->GetCurrentMetaData();
    std::string deviceId = (info && info->Has(vtkCompositeDataSet::NAME()))
      ? info->Get(vtkCompositeDataSet::NAME())
      : std::string("Lidar") + std::to_string(inputId);
    auto timestamp = current->GetPointData()->GetArray(this->GetTimeArrayName(current).c_str());
    if (!timestamp)
    {
      vtkErrorMacro("No timestamps array found for LiDAR sensor: " << deviceId);
      return false;
    }
    // Get current frame time
    double currentFrameTime = timestamp->GetRange()[1];
    if (this->RelativeTimestamp)
    {
      currentFrameTime = this->GetTimeFromFieldData(current);
    }
    // Update input data if frame time is updated
    if (this->FrameTime.find(deviceId) == this->FrameTime.end() ||
      currentFrameTime > this->FrameTime[deviceId])
    {
      vecPolydata[deviceId] = current;
    }
    this->FrameTime[deviceId] = currentFrameTime;

    it->GoToNextItem();
    ++inputId;
  }
  it->Delete();

  return true;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the time and force it
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // Get the input pointcloud and trajectory
  PointCloudMap vecPointcloud;
  bool isInitialized = this->InitPointCloud(inputVector[0]->GetInformationObject(0), vecPointcloud);
  vtkPolyData* trajectory = vtkPolyData::GetData(inputVector[1]->GetInformationObject(0));
  if (!isInitialized || !trajectory)
  {
    vtkErrorMacro("No input data");
    return 0;
  }

  // Initialize the data if necessary (first iteration)
  if (!this->Initialized)
  {
    // Initialize the data
    this->InitializeData(trajectory, vecPointcloud.begin()->second);
    this->Initialized = true;
  }

  if (this->AutoComputeBounds && !this->AreBoundsComputed)
  {
    // If the bounds have not been computed yet, and autoComputeBounds is enabled, compute them
    // before aggregating the points
    // /!\ This is called for all frames at the start of the filter.
    //     (Does not work with online SLAM)
    return this->AutoComputeVoxelBounds(request, inInfo, vecPointcloud);
  }
  return this->AggregatePoints(request, inInfo, outputVector, vecPointcloud);
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOnline::InitializeData(vtkPolyData* trajectory,
  vtkPolyData* pointcloud)
{
  vtkSmartPointer<vtkPolyData> localTrajectory = vtkSmartPointer<vtkPolyData>::New();
  localTrajectory->DeepCopy(trajectory);
  // Check whether or not the trajectory contain larege scale
  this->CheckAndRemoveOffsetFromTrajectory(localTrajectory);

  // Create the temporal transform
  auto temporalTransform = vtkTemporalTransforms::CreateFromPolyData(localTrajectory);
  // Fill the interpolator
  this->Interpolator = temporalTransform->CreateInterpolator();
  this->Interpolator->SetInterpolationType(this->InterpolationType);

  // Check whether or not the trajectory time is considered continuous with a default tolerance of
  // 0.5s
  this->ContinuousTrajectory = this->Interpolator->IsTimeContinuous();

  // Get the time array
  this->TimeArrayName = this->GetTimeArrayName(pointcloud);

  // Get the timestamp array
  auto timestamp = pointcloud->GetPointData()->GetArray(this->TimeArrayName.c_str());
  if (!timestamp)
  {
    vtkErrorMacro("No TimeStamp array found.");
    return;
  }

  // Get the conversion factor between the time unit of the trajectory and the pointcloud
  if (this->AutoDetectTimeUnitConversion)
  {
    this->ConversionFactorToSecond =
      this->ComputeTimeUnitConversion(temporalTransform->GetTimeArray(), timestamp);
  }
  else
  {
    this->ConversionFactorToSecond = this->CustomConversionFactorToSecond;
  }

  // Set initial number of points to allocate in the voxel grid and the free points
  this->VoxelGrid->SetInitialNumberOfPoints(this->DefaultNumberOfPoints);
  this->MergePointsToPolyDataHelper->SetInitialNumberOfPoints(this->NumberOfPoints);
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOnline::CheckAndRemoveOffsetFromTrajectory(
  vtkPolyData* localTrajectory)
{
  this->IsOffsetRemoved = false;

  vtkPoints* trajPositions = localTrajectory->GetPoints();
  if (!trajPositions || trajPositions->GetNumberOfPoints() == 0)
  {
    vtkWarningMacro("Trajectory is missing X, Y or Z array or is empty.");
    return;
  }

  // Get the first position as offset
  trajPositions->GetPoint(0, this->OffsetOrigin);
  double& x0 = this->OffsetOrigin[0];
  double& y0 = this->OffsetOrigin[1];
  double& z0 = this->OffsetOrigin[2];

  double distToZero = std::sqrt(x0 * x0 + y0 * y0 + z0 * z0);
  if (distToZero < 10000.0)
  {
    return;
  }
  // Remove large offset
  for (vtkIdType i = 0; i < trajPositions->GetNumberOfPoints(); ++i)
  {
    double p[3];
    trajPositions->GetPoint(i, p);
    p[0] -= x0;
    p[1] -= y0;
    p[2] -= z0;
    trajPositions->SetPoint(i, p);
  }

  this->IsOffsetRemoved = true;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::AutoComputeVoxelBounds(vtkInformation* request,
  vtkInformation* inInfo,
  PointCloudMap& vecPointcloud)
{
  for (const auto& [deviceId, pointcloud] : vecPointcloud)
  {
    double currentBounds[6];
    pointcloud->GetBounds(currentBounds);

    // Get timestamp array
    auto timestamp =
      pointcloud->GetPointData()->GetArray(this->GetTimeArrayName(pointcloud).c_str());

    // Get the current timestamp in seconds
    double currentTimestamp =
      timestamp->GetTuple1(0) * this->ConversionFactorToSecond + this->TimeOffset;
    if (this->RelativeTimestamp)
    {
      currentTimestamp += this->FrameTime[deviceId];
    }

    // Get the right transform
    auto transform = vtkSmartPointer<vtkTransform>::New();
    this->Interpolator->InterpolateTransform(currentTimestamp, transform);
    transform->Update();

    // Update the number of points
    this->NumberOfPoints += pointcloud->GetPoints()->GetNumberOfPoints();

    // Get the bounding box of the current pointcloud
    vtkBoundingBox boundingBox;
    boundingBox.SetBounds(currentBounds);
    vtkBoundingBox transformedBoundingBox;

    // Transform 8 corners of the bounding box
    for (int i = 0; i < 8; i++)
    {
      double point[3];
      boundingBox.GetCorner(i, point);

      double transformedPoint[3];
      transform->TransformPoint(point, transformedPoint);
      if (this->IsOffsetRemoved)
      {
        transformedPoint[0] += this->OffsetOrigin[0];
        transformedPoint[1] += this->OffsetOrigin[1];
        transformedPoint[2] += this->OffsetOrigin[2];
      }
      transformedBoundingBox.AddPoint(transformedPoint);
    }

    // Update the bounds
    for (int i = 0; i < 6; i++)
    {
      this->Bounds[i] = i % 2 == 0 ? std::min(this->Bounds[i], transformedBoundingBox.GetBound(i))
                                   : std::max(this->Bounds[i], transformedBoundingBox.GetBound(i));
    }
  }

  // Update the state of the autoComputeBounds process
  this->UpdateAutoComputeBoundsProgress(inInfo);

  // Continue the pipeline loop
  request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOnline::UpdateAutoComputeBoundsProgress(vtkInformation* inInfo)
{
  int timeStepNumber = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (this->CurrentFrame >= timeStepNumber - 1)
  {
    this->AreBoundsComputed = true;
    this->CurrentFrame = 0;
    return;
  }

  this->CurrentFrame += 1;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::AggregatePoints(vtkInformation* request,
  vtkInformation* inInfo,
  vtkInformationVector* outputVector,
  PointCloudMap& vecPointcloud)
{
  // The bounds are computed only once for the first frame
  if (!this->IsVoxelGridFilterInitialized)
  {
    auto bounds = this->AutoComputeBounds ? this->Bounds.data() : this->CustomBounds;
    this->VoxelGrid->SetBounds(bounds);
    this->IsVoxelGridFilterInitialized = true;
  }

  // Transform the points of the pointcloud with the trajectory and add them to the voxel grid
  if (!this->TransformAndAddPoints(vecPointcloud))
  {
    vtkErrorMacro(<< "Aggregation failed.");
    return 0;
  }

  // Get the outputs from the voxel grid and the free points and merge them
  vtkNew<vtkAppendPolyData> appendFilter;
  if (this->IsVoxelGridFilterUsed)
  {
    if (this->MinFramesPerVoxel > 0)
    {
      appendFilter->AddInputData(this->VoxelGrid->GetFilteredOutput(this->MinFramesPerVoxel));
    }
    else
    {
      appendFilter->AddInputData(this->VoxelGrid->GetOutput());
    }
  }
  // The free points are added after the voxel grid
  appendFilter->AddInputData(this->MergePointsToPolyDataHelper->GetOutput());
  appendFilter->Update();
  if (this->DisplayOutput)
  {
    vtkPolyData* output = vtkPolyData::GetData(outputVector);
    output->ShallowCopy(appendFilter->GetOutput());
  }

  request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::TransformAndAddPoints(PointCloudMap& vecPointcloud)
{
  for (const auto& [deviceId, pointcloud] : vecPointcloud)
  {
    // Get timestamp array
    auto timestamp =
      pointcloud->GetPointData()->GetArray(this->GetTimeArrayName(pointcloud).c_str());
    double frameTime = this->RelativeTimestamp ? this->FrameTime[deviceId] : 0.;

    // Check timestamps of the first and the last points
    std::array<vtkIdType, 2> pointIdx = { 0, pointcloud->GetNumberOfPoints() - 1 };
    for (const auto& idx : pointIdx)
    {
      double currentTimestamp =
        timestamp->GetTuple1(idx) * this->ConversionFactorToSecond + this->TimeOffset + frameTime;
      if (!this->Interpolator->IsTimeInRange(currentTimestamp))
      {
        vtkWarningMacro("The trajectory does not have a valid time for the current timestamp, "
          << "interpolation might be invalid.");
      }
    }

    for (vtkIdType i = 0; i < pointcloud->GetNumberOfPoints(); i++)
    {
      // Get the current timestamp in seconds
      double currentTimestamp =
        timestamp->GetTuple1(i) * this->ConversionFactorToSecond + this->TimeOffset + frameTime;

      // Get the right transform
      auto transform = vtkSmartPointer<vtkTransform>::New();
      this->Interpolator->InterpolateTransform(currentTimestamp, transform);
      transform->Update();

      // Transform the point
      double p[3];
      pointcloud->GetPoint(i, p);
      transform->TransformPoint(p, p);
      // If the offset is removed from the input trajectory, re-apply this offset
      if (this->IsOffsetRemoved)
      {
        p[0] += this->OffsetOrigin[0];
        p[1] += this->OffsetOrigin[1];
        p[2] += this->OffsetOrigin[2];
      }

      if (this->IsVoxelGridFilterUsed)
      {
        // Add the point to the voxel grid
        this->VoxelGrid->AddPoint(pointcloud, i, p);
      }
      else
      {
        // Add a free point (not filtered by the voxel grid)
        this->MergePointsToPolyDataHelper->AddPoint(pointcloud, i, p);
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
std::string vtkAggregatePointsFromTrajectoryOnline::GetTimeArrayName(vtkPolyData* poly)
{
  if (this->AutoDetectTimeArray)
  {
    std::string tempTimeArrayName = this->DetectTimeArray(poly);
    return tempTimeArrayName.empty() ? this->CustomTimeArrayName : tempTimeArrayName;
  }
  return this->CustomTimeArrayName;
}

//----------------------------------------------------------------------------
std::string vtkAggregatePointsFromTrajectoryOnline::DetectTimeArray(vtkPolyData* poly)
{
  // Loop over all the arrays
  for (int i = 0; i < poly->GetPointData()->GetNumberOfArrays(); i++)
  {
    char* arrayName = poly->GetPointData()->GetArray(i)->GetName();
    std::string arrayNameStr(arrayName);
    std::size_t found_Time = arrayNameStr.find("Time");
    std::size_t found_time = arrayNameStr.find("time");
    if (found_Time != std::string::npos || found_time != std::string::npos)
    {
      return arrayNameStr;
    }
  }
  return "";
}

//----------------------------------------------------------------------------
double vtkAggregatePointsFromTrajectoryOnline::GetTimeFromFieldData(vtkPolyData* pointcloud)
{
  double timestamp = -1.;
  // Use timestamps from field data if it is available
  vtkFieldData* fieldData = pointcloud->GetFieldData();
  if (fieldData)
  {
    vtkDataArray* timestampArray = fieldData->GetArray(this->ReferenceTimeName.c_str());
    if (timestampArray && timestampArray->GetNumberOfTuples() > 0)
    {
      timestamp = timestampArray->GetTuple1(0);
    }
  }
  return timestamp;
}

//----------------------------------------------------------------------------
double vtkAggregatePointsFromTrajectoryOnline::ComputeTimeUnitConversion(
  vtkDataArray* trajTimeArray,
  vtkDataArray* pcTimeArray)
{
  // Check if the trajectory time array is empty
  if (trajTimeArray->GetNumberOfComponents() != 1 || pcTimeArray->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("One of the selected time array is not a scalar.");
    return 1e-6;
  }
  // Check if the trajectory time array is empty
  if (trajTimeArray->GetNumberOfTuples() == 0 || pcTimeArray->GetNumberOfTuples() == 0)
  {
    vtkErrorMacro("One of the selected time arrays is empty.");
    return 1e-6;
  }

  double conversionFactor = 1.0;

  if (this->RelativeTimestamp)
  {
    double* range = pcTimeArray->GetRange();
    double duration = std::abs(range[1] - range[0]);

    // We suppose the duration time is contained between 20ms and 0.9s.
    while (conversionFactor * duration > 0.9) // Min = ~1 Hz
    {
      conversionFactor *= 1e-3;
    }
  }
  else
  {
    // Get the first timestamp
    double firstTrajTimestamp = trajTimeArray->GetTuple1(0);
    double firstPCTimestamp = pcTimeArray->GetTuple1(0);

    if (firstPCTimestamp < 1e-6)
    {
      vtkErrorMacro("The first timestamp of the point cloud is too small.");
      return 1e-6;
    }

    double div = firstTrajTimestamp / firstPCTimestamp;
    // Get the order of magnitude of conversion
    double order = std::floor(std::log10(div));
    // Round the power to the nearest multiple of 3
    order = std::round(order / 3) * 3;
    // Get the conversion factor
    conversionFactor = std::pow(10, order);
  }
  return conversionFactor;
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOnline::Clear()
{
  this->FrameTime.clear();
  this->VoxelGrid->Clear();
  this->MergePointsToPolyDataHelper->Clear();
  this->CurrentFrame = 0;
  this->IsVoxelGridFilterInitialized = false;
  this->Initialized = false;
  this->NumberOfPoints = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOnline::FreeUnusedMemory()
{
  this->VoxelGrid->FreeUnusedMemory();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOnline::SetVoxelSamplingMode(int mode)
{
  // Clear the voxel grid to avoid mixing points from different sampling modes
  this->Clear();
  this->VoxelGrid->SetSampling(mode);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOnline::SetVoxelLeafSize(double size)
{
  this->VoxelGrid->SetLeafSize(size);
  this->Modified();
}
