/*=========================================================================

  Program:   LidarView
  Module:    vtkTrailingFrame.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTrailingFrame.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTrailingFrame)

//----------------------------------------------------------------------------
void vtkTrailingFrame::SetNumberOfTrailingFrames(const unsigned int value)
{
  if (this->NumberOfTrailingFrames != value)
  {
    this->NumberOfTrailingFrames = value;
    this->CacheTimeRange[0] = -1;
    this->CacheTimeRange[1] = -1;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkTrailingFrame::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTrailingFrame::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // In stream mode we don't need the RequestUpdateExtent step
  if (this->UseStreamMode)
  {
    return 1;
  }

  // Get the available time steps from source
  // This is done every time the number of timesteps is changed in the UI
  unsigned int nbTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (this->TimeSteps.empty() || this->TimeSteps.size() != nbTimeSteps)
  {
    double* timeStepsData = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeSteps.assign(timeStepsData, timeStepsData + nbTimeSteps);
  }

  // Workaround to handle that multiple RequestUpdateExtent can be call
  // The filter made the assumption that each RequestUpdateExtent is follow by a RequestData
  if (this->LastCallWasRequestUpdateExtentCall)
  {
    return 1;
  }
  this->LastCallWasRequestUpdateExtentCall = true;

  // If the TimeSteps size is still zero, it means
  // that no time_steps has been filled by the reader.
  // Hence, no lidar data was stored in the .pcap.
  // It could also means that the user is tring to use
  // this filter in stream mode without the option.
  if (this->TimeSteps.size() == 0)
  {
    vtkGenericWarningMacro("No time steps are available, it could either mean:\n"
      << "- If you are in playback mode: the reader was not able parse the data "
      << "no lidar data are present in the .pcap file\n"
      << "- If you are in stream mode: you need to activate the UseStreamMode option.");
    this->LastTimeProcessedIndex =
      (LastTimeProcessedIndex + 1) % (this->NumberOfTrailingFrames + 1);
    return 1;
  }

  // Trigger when all trailing frames has been setted
  if (this->FirstFilterIteration)
  {
    // Delete the cache so it cannot be used
    if (!this->UseCache)
    {
      this->CacheTimeRange[0] = -1;
      this->CacheTimeRange[1] = -1;
      this->Cache->Initialize();
    }
    // Save current pipeline time step
    this->PipelineTime = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // get the index corresponding to the requested pipeline time
    // find the index of the first time step that is not less than pipeline time
    this->PipelineIndex = std::distance(this->TimeSteps.begin(),
      std::lower_bound(this->TimeSteps.begin(), this->TimeSteps.end(), this->PipelineTime));
    // check if the previous index is closer
    if (this->PipelineIndex > 0)
    {
      double currentDiffTime = this->TimeSteps[this->PipelineIndex] - this->PipelineTime;
      double previousDiffTime = this->PipelineTime - this->TimeSteps[this->PipelineIndex - 1];
      if (currentDiffTime > previousDiffTime)
      {
        this->PipelineIndex--;
      }
    }
    // save old TimeRange and update new one
    int previousCacheTimeRange[2] = { this->CacheTimeRange[0], this->CacheTimeRange[1] };
    this->CacheTimeRange[0] =
      std::max(static_cast<int>(this->PipelineIndex - this->NumberOfTrailingFrames), 0);
    this->CacheTimeRange[1] = this->PipelineIndex + 1;

    // handle case when changing NumberOfTrailingFrame
    if (previousCacheTimeRange[1] == -1)
    {
      // move the old  timestep to the right location
      vtkNew<vtkMultiBlockDataSet> oldCache;
      oldCache->ShallowCopy(this->Cache.GetPointer());
      int previousNumberOfTrailingFrame = oldCache->GetNumberOfBlocks() - 1;
      this->Cache->Initialize();
      for (this->LastTimeProcessedIndex = this->CacheTimeRange[1] - 1;
           this->LastTimeProcessedIndex >
             this->CacheTimeRange[1] - 1 - previousNumberOfTrailingFrame &&
           this->LastTimeProcessedIndex > this->CacheTimeRange[0];
           this->LastTimeProcessedIndex--)
      {
        unsigned int previousIndex =
          this->LastTimeProcessedIndex % (previousNumberOfTrailingFrame + 1);
        unsigned int newIndex = this->LastTimeProcessedIndex % (this->NumberOfTrailingFrames + 1);
        this->Cache->SetBlock(newIndex, oldCache->GetBlock(previousIndex));
      }
      this->Direction = DirectionType::BACKWARD;
    }
    // handle case when jumping backward
    else if (this->CacheTimeRange[1] < previousCacheTimeRange[1])
    {
      this->Direction = DirectionType::BACKWARD;
      this->LastTimeProcessedIndex =
        std::max(0, std::min(this->CacheTimeRange[1], previousCacheTimeRange[0]) - 1);
    }
    // handle case when jumping forward
    else if (this->CacheTimeRange[1] > previousCacheTimeRange[1])
    {
      this->Direction = DirectionType::FORWARD;
      this->LastTimeProcessedIndex = std::max(this->CacheTimeRange[0], previousCacheTimeRange[1]);
    }
    this->FirstFilterIteration = false;
  }
  else
  {
    this->LastTimeProcessedIndex += this->Direction;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
    this->TimeSteps[this->LastTimeProcessedIndex]);

  return 1;
}

//----------------------------------------------------------------------------
int vtkTrailingFrame::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->UseStreamMode)
  {
    return this->ProcessStreamingMode(request, inputVector, outputVector);
  }
  else
  {
    return this->ProcessReadingMode(request, inputVector, outputVector);
  }
}

//----------------------------------------------------------------------------
int vtkTrailingFrame::ProcessStreamingMode(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector);

  vtkNew<vtkPolyData> currentFrame;
  currentFrame->ShallowCopy(input);

  unsigned int StreamNumberOfTrailingFrames = this->NumberOfTrailingFrames + 1;

  // Delete excess frames
  if (this->CurrentTrailingFramesNumber > StreamNumberOfTrailingFrames)
  {
    this->Cache->SetNumberOfBlocks(StreamNumberOfTrailingFrames + 1);
    this->CurrentTrailingFramesNumber = StreamNumberOfTrailingFrames;
  }

  this->Cache->SetBlock(this->CurrentTrailingFramesNumber, currentFrame.GetPointer());

  // Re-order output blocks, to do a queue FIFO
  if (this->CurrentTrailingFramesNumber == StreamNumberOfTrailingFrames)
  {
    for (unsigned int i = 0; i < this->CurrentTrailingFramesNumber; i++)
    {
      this->Cache->SetBlock(i, this->Cache->GetBlock(i + 1));
    }
  }

  if (this->CurrentTrailingFramesNumber < StreamNumberOfTrailingFrames)
  {
    this->CurrentTrailingFramesNumber += 1;
  }

  output->ShallowCopy(this->Cache.GetPointer());
  return 1;
}

//----------------------------------------------------------------------------
int vtkTrailingFrame::ProcessReadingMode(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector);

  // Workaround to handle that multiple RequestUpdateExtent can be call
  // The filter made the assumption that each RequestUpdateExtent is follow by a RequestData
  this->LastCallWasRequestUpdateExtentCall = false;

  // If the TimeSteps size is still zero, it means
  // that no time_steps has been filled by the reader.
  // Hence, no lidar data was stored in the .pcap.
  // It could also means that the user is tring to use
  // this filter in stream mode without the option.
  if (this->TimeSteps.size() == 0)
  {
    // Stop the pipeline loop
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    return 1;
  }

  bool isCacheCompletelyUpdated = this->Direction == DirectionType::FORWARD
    ? this->LastTimeProcessedIndex == this->CacheTimeRange[1] - 1
    : this->LastTimeProcessedIndex == this->CacheTimeRange[0];

  if (isCacheCompletelyUpdated)
  {
    // Stop the pipeline loop
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());

    // reset block that should be empty
    for (unsigned int i = this->PipelineIndex + 1; i < this->NumberOfTrailingFrames + 1; i++)
    {
      int index = i % (this->NumberOfTrailingFrames + 1);
      this->Cache->SetBlock(index, nullptr);
    }

    // reset some variable and pipeline time
    this->FirstFilterIteration = true;
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->PipelineTime);
  }
  else
  {
    // force the pipeline loop
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  // copy the input in the multiblock
  vtkNew<vtkPolyData> currentFrame;
  currentFrame->ShallowCopy(input);
  if (this->NumberOfTrailingFrames != 0)
  {
    unsigned int index = this->LastTimeProcessedIndex % (this->NumberOfTrailingFrames + 1);
    this->Cache->SetBlock(index, currentFrame.GetPointer());
  }
  // handle case when no trailing frame
  else
  {
    this->Cache->SetBlock(0, currentFrame.GetPointer());
  }
  output->ShallowCopy(this->Cache.GetPointer());

  // re-order output blocks so that:
  //    current frame => 0
  //    current frame - 1 => 1
  //    current frame - 2 => 2
  // ...
  int n = this->NumberOfTrailingFrames + 1;
  if (n > 0)
  {
    int current_frame_index = this->PipelineIndex % n;
    for (int i = 1; i <= n; ++i)
    {
      output->SetBlock(n - i, this->Cache->GetBlock((current_frame_index + i) % n));
    }
  }
  return 1;
}
