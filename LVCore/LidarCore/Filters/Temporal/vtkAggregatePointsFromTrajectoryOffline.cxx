/*=========================================================================

  Program:   LidarView
  Module:    vtkAggregatePointsFromTrajectoryOffline.cxx

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
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

// Local includes
#include "vtkAggregatePointsFromTrajectoryOffline.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAggregatePointsFromTrajectoryOffline)

//----------------------------------------------------------------------------
vtkAggregatePointsFromTrajectoryOffline::vtkAggregatePointsFromTrajectoryOffline()
{
  this->FirstFrame = this->AllFrames ? 0 : this->FirstFrame;
  this->LastFrame = this->AllFrames ? 0 : this->LastFrame;
  this->CurrentFrame = this->FirstFrame;
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOffline::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
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
int vtkAggregatePointsFromTrajectoryOffline::RequestData(vtkInformation* request,
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

  if (!vtkAggregatePointsFromTrajectoryOnline::RequestData(request, inputVector, outputVector))
  {
    // Stop the pipeline loop
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkAggregatePointsFromTrajectoryOffline::UpdateAutoComputeBoundsProgress(
  vtkInformation* inInfo)
{
  // Relaunch the pipeline if necessary
  int timeStepNumber = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int lastFrame = this->AllFrames ? timeStepNumber - 1 : this->LastFrame;
  bool lastIteration = CurrentFrame - 1 >= lastFrame;
  if (lastIteration)
  {
    this->AreBoundsComputed = true;
    this->CurrentFrame = this->FirstFrame;
  }
  else
  {
    // Update the current frame to the next one
    this->CurrentFrame += this->StepSize;
  }
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOffline::AggregatePoints(vtkInformation* request,
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

    // Set initial number of points to allocate in the voxel grid and the free points
    this->VoxelGrid->SetInitialNumberOfPoints(this->DefaultNumberOfPoints);
    this->MergePointsToPolyDataHelper->SetInitialNumberOfPoints(this->NumberOfPoints);
  }

  // Transform the points of the pointcloud with the trajectory and add them to the voxel grid
  if (!this->TransformAndAddPoints(timestamp, pointcloud))
  {
    vtkErrorMacro(<< "Aggregation failed.");
    return 0;
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
