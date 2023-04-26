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

// VTK includes
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
    this->VoxelGrid->SetBounds(this->Bounds);
    this->Initialized = true;

    // Create the temporal transform
    auto temporalTransform = vtkTemporalTransforms::CreateFromPolyData(trajectory);
    // Fill the interpolator
    this->Interpolator = temporalTransform->CreateInterpolator();
    this->Interpolator->SetInterpolationType(this->InterpolationType);

    // Check whether or not the trajectory time is considered continuous with a default tolerance of
    // 0.5s
    this->ContinuousTrajectory = this->Interpolator->IsTimeContinuous();
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
    return 1;
  }

  for (vtkIdType i = 0; i < pointcloud->GetNumberOfPoints(); i++)
  {
    // Get the curent timestamp in seconds
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

    // Add the point to the voxel grid
    if (!this->VoxelGrid->AddPoint(pointcloud, i, p))
    {
      vtkWarningMacro("Add point failed");
    }
  }

  // Relaunch the pipeline if necessary
  int lastFrame = this->AllFrames ? timeStepNumber - 1 : this->LastFrame;

  if (this->CurrentFrame >= lastFrame)
  {
    // Stop the pipeline loop
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());

    // The voxel grid allocates extra memory to avoid reallocation, so memory needs to be freed
    this->VoxelGrid->FreeUnusedMemory();

    // Get the output from the voxel grid
    vtkPolyData* output = vtkPolyData::GetData(outputVector);
    output->ShallowCopy(this->VoxelGrid->GetOutput());

    // Reset attributes
    this->VoxelGrid->Clear();
    this->CurrentFrame = this->AllFrames ? 0 : this->FirstFrame;
    this->Initialized = false;
    return 1;
  }

  // Continue the pipeline loop
  request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  this->CurrentFrame += this->StepSize;

  return 1;
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
