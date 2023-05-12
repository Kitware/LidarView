/*=========================================================================

  Program:   LidarView
  Module:    vtkAggregatePointsFromTrajectory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// STL includes
#include <algorithm>
#include <cmath>

// VTK includes
#include <vtkAppendPolyData.h>
#include <vtkBoundingBox.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTemporalTransforms.h>
#include <vtkTransform.h>

// Local includes
#include "vtkAggregatePointsFromTrajectory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAggregatePointsFromTrajectory)

//----------------------------------------------------------------------------
vtkAggregatePointsFromTrajectory::vtkAggregatePointsFromTrajectory()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  // Initialize the current first frame index
  this->CurrentFrame = this->AllFrames ? 0 : this->FirstFrame;
}

//-----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectory::FillInputPortInformation(int port, vtkInformation* info)
{
  // Pointcloud data
  if (port == 0 || port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectory::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // Get the time and force it
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  double time = *(inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) + this->CurrentFrame);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
  return 1;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectory::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the time and force it
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  int timeStepNumber = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  // Check parameters validity
  if (this->StepSize <= 0)
  {
    vtkErrorMacro("StepSize must be greater than zero!");
    return 0;
  }
  // Check validity of FirstFrame and LastFrame if AllFrames is disabled
  if (!this->AllFrames)
  {
    if (this->LastFrame < 0 || this->FirstFrame < 0)
    {
      vtkErrorMacro("FirstFrame and LastFrame must be positive integers!");
      return 0;
    }
    if (this->FirstFrame > timeStepNumber - 1 || this->LastFrame > timeStepNumber - 1)
    {
      vtkErrorMacro("The dataset only has " << timeStepNumber << " frames!");
      return 0;
    }
    if (this->LastFrame < this->FirstFrame)
    {
      vtkErrorMacro("The last frame must come after the first frame!");
      return 0;
    }
  }

  // Get the input pointcloud and trajectory
  vtkPolyData* pointcloud = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData* trajectory = vtkPolyData::GetData(inputVector[1]->GetInformationObject(0));

  // In case the input is a multiblock dataset, the first block is extracted
  if (!pointcloud)
  {
    vtkMultiBlockDataSet* mb =
      vtkMultiBlockDataSet::GetData(inputVector[0]->GetInformationObject(0));
    // Extract first block if it is a vtkPolyData
    pointcloud = vtkPolyData::SafeDownCast(mb->GetBlock(0));
  }

  if (!pointcloud || !trajectory)
  {
    vtkErrorMacro("No input data");
    return 0;
  }

  // Initialize the data if necessary (first iteration)
  if (!this->Initialized)
  {
    // Initialize the data
    this->InitializeData(trajectory, pointcloud);
    this->Initialized = true;
  }

  if (!this->Interpolator)
  {
    vtkErrorMacro("Interpolator not initialized");
    return 0;
  }

  auto timestamp = pointcloud->GetPointData()->GetArray(this->TimeArrayName.c_str());
  if (!timestamp)
  {
    vtkErrorMacro("No TimeStamp array selected.");
    return 0;
  }

  if (this->AutoComputeBounds && !this->AreBoundsComputed)
  {
    // If the bounds have not been computed yet, and autoComputeBounds is enabled, compute them
    // before aggregating the points
    return this->AutoComputeVoxelBounds(request, inInfo, pointcloud, timestamp);
  }
  else
  {
    return this->AggregatePoints(request, inInfo, outputVector, pointcloud, timestamp);
  }
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectory::InitializeData(vtkPolyData* trajectory,
  vtkPolyData* pointcloud)
{
  // Create the temporal transform
  auto temporalTransform = vtkTemporalTransforms::CreateFromPolyData(trajectory);
  // Fill the interpolator
  this->Interpolator = temporalTransform->CreateInterpolator();
  this->Interpolator->SetInterpolationType(this->InterpolationType);

  // Check whether or not the trajectory time is considered continuous with a default tolerance of
  // 0.5s
  this->ContinuousTrajectory = this->Interpolator->IsTimeContinuous();

  // Get the time array
  if (this->AutoDetectTimeArray)
  {
    std::string tempTimeArrayName = this->DetectTimeArray(pointcloud);
    this->TimeArrayName = tempTimeArrayName.empty() ? this->CustomTimeArrayName : tempTimeArrayName;
  }
  else
  {
    this->TimeArrayName = this->CustomTimeArrayName;
  }

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

  // Reset the bounds
  this->Bounds = { 0, 0, 0, 0, 0, 0 };

  // Set initial number of points to allocate in the voxel grid and the free points
  this->VoxelGrid->SetInitialNumberOfPoints(this->DefaultNumberOfPoints);
  this->MergePointsToPolyDataHelper->SetInitialNumberOfPoints(this->NumberOfPoints);
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectory::AutoComputeVoxelBounds(vtkInformation* request,
  vtkInformation* inInfo,
  vtkPolyData* pointcloud,
  vtkDataArray* timestamp)
{
  double currentBounds[6];
  pointcloud->GetBounds(currentBounds);

  // Get the current timestamp in seconds
  double currentTimestamp =
    timestamp->GetTuple1(0) * this->ConversionFactorToSecond + this->TimeOffset;

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
    transformedBoundingBox.AddPoint(transformedPoint);
  }

  // Update the bounds
  for (int i = 0; i < 6; i++)
  {
    this->Bounds[i] = i % 2 == 0 ? std::min(this->Bounds[i], transformedBoundingBox.GetBound(i))
                                 : std::max(this->Bounds[i], transformedBoundingBox.GetBound(i));
  }

  // Continue the pipeline loop
  request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

  // Relaunch the pipeline if necessary
  int timeStepNumber = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int lastFrame = this->AllFrames ? timeStepNumber - 1 : this->LastFrame;

  if (this->CurrentFrame >= lastFrame)
  {
    this->AreBoundsComputed = true;
    this->CurrentFrame = this->AllFrames ? 0 : this->FirstFrame;
    return 1;
  }

  // Update the current frame to the next one
  this->CurrentFrame += this->StepSize;
  return 1;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectory::AggregatePoints(vtkInformation* request,
  vtkInformation* inInfo,
  vtkInformationVector* outputVector,
  vtkPolyData* pointcloud,
  vtkDataArray* timestamp)
{
  int firstFrame = this->AllFrames ? 0 : this->FirstFrame;
  // The bounds are computed only once before the aggregation
  if (this->CurrentFrame == firstFrame)
  {
    auto bounds = this->AutoComputeBounds ? this->Bounds.data() : this->CustomBounds;
    this->VoxelGrid->SetBounds(bounds);
  }

  for (vtkIdType i = 0; i < pointcloud->GetNumberOfPoints(); i++)
  {
    // Get the current timestamp in seconds
    double currentTimestamp =
      timestamp->GetTuple1(i) * this->ConversionFactorToSecond + this->TimeOffset;

    // If the trajectory is continuous, the current time must simply belong to the trajectory
    if (this->ContinuousTrajectory && !this->Interpolator->IsTimeInRange(currentTimestamp))
    {
      continue;
    }
    // If the trajectory is not continuous, the current time must not be in a discontinuity of
    // the trajectory
    else if (!this->ContinuousTrajectory && !this->Interpolator->IsTimeValid(currentTimestamp))
    {
      continue;
    }

    // Get the right transform
    auto transform = vtkSmartPointer<vtkTransform>::New();
    this->Interpolator->InterpolateTransform(currentTimestamp, transform);
    transform->Update();

    // Transform the point
    double p[3];
    pointcloud->GetPoint(i, p);
    transform->TransformPoint(p, p);

    bool addPointSuccess;

    if (this->IsVoxelGridFilterUsed)
    {
      // Add the point to the voxel grid
      addPointSuccess = this->VoxelGrid->AddPoint(pointcloud, i, p);
    }
    else
    {
      // Add a free point (not filtered by the voxel grid)
      addPointSuccess = this->MergePointsToPolyDataHelper->AddPoint(pointcloud, i, p);
    }

    if (!addPointSuccess)
    {
      vtkWarningMacro("Add point failed");
    }
  }

  // Relaunch the pipeline if necessary
  int timeStepNumber = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int lastFrame = this->AllFrames ? timeStepNumber - 1 : this->LastFrame;

  if (this->CurrentFrame >= lastFrame)
  {
    // Stop the pipeline loop
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());

    // Extra memory is allocated in the vtkVoxelGridProcessor and the vtkMergePointsToPolyDataHelper
    // outputs to avoid reallocation, so memory needs to be freed
    this->VoxelGrid->FreeUnusedMemory();
    this->MergePointsToPolyDataHelper->FreeUnusedMemory();

    // Get the outputs from the voxel grid and the free points and merge them
    vtkNew<vtkAppendPolyData> appendFilter;
    appendFilter->AddInputData(this->VoxelGrid->GetOutput());
    // The free points are added after the voxel grid
    appendFilter->AddInputData(this->MergePointsToPolyDataHelper->GetOutput());
    appendFilter->Update();
    vtkPolyData* output = vtkPolyData::GetData(outputVector);
    output->ShallowCopy(appendFilter->GetOutput());

    // Reset attributes
    this->VoxelGrid->Clear();
    this->MergePointsToPolyDataHelper->Clear();
    this->CurrentFrame = this->AllFrames ? 0 : this->FirstFrame;
    this->Initialized = false;
    this->AreBoundsComputed = false;
    this->NumberOfPoints = 0;
    return 1;
  }

  // Continue the pipeline loop
  request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  this->CurrentFrame += this->StepSize;

  return 1;
}

//----------------------------------------------------------------------------
std::string vtkAggregatePointsFromTrajectory::DetectTimeArray(vtkPolyData* poly)
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
double vtkAggregatePointsFromTrajectory::ComputeTimeUnitConversion(vtkDataArray* trajTimeArray,
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
  double conversionFactor = std::pow(10, order);
  return conversionFactor;
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectory::SetVoxelSamplingMode(int mode)
{
  this->VoxelGrid->SetSampling(mode);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectory::SetVoxelLeafSize(double size)
{
  this->VoxelGrid->SetLeafSize(size);
  this->Modified();
}
