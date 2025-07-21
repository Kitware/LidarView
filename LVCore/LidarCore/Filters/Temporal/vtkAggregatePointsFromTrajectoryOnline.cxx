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
}

//-----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::FillInputPortInformation(int port, vtkInformation* info)
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
int vtkAggregatePointsFromTrajectoryOnline::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the time and force it
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

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

  auto timestamp = vtkDataArray::SafeDownCast(
    pointcloud->GetPointData()->GetAbstractArray(this->TimeArrayName.c_str()));
  if (!timestamp)
  {
    vtkErrorMacro("No TimeStamp array selected.");
    return 0;
  }

  if (this->AutoComputeBounds && !this->AreBoundsComputed)
  {
    // If the bounds have not been computed yet, and autoComputeBounds is enabled, compute them
    // before aggregating the points
    // /!\ This is called for all frames at the start of the filter.
    //     (Does not work with online SLAM)
    return this->AutoComputeVoxelBounds(request, inInfo, pointcloud, timestamp);
  }
  return this->AggregatePoints(request, inInfo, outputVector, pointcloud, timestamp);
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

  vtkLog(INFO,
    << "The input trajectory seems to be georeferenced. \n"
    << "A local trajectory has been computed by removing the offset: ( " << x0 << "," << y0 << ","
    << z0 << " )");
}

//----------------------------------------------------------------------------
int vtkAggregatePointsFromTrajectoryOnline::AutoComputeVoxelBounds(vtkInformation* request,
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
  vtkPolyData* pointcloud,
  vtkDataArray* timestamp)
{
  // The bounds are computed only once for the first frame
  if (!this->IsVoxelGridFilterInitialized)
  {
    auto bounds = this->AutoComputeBounds ? this->Bounds.data() : this->CustomBounds;
    this->VoxelGrid->SetBounds(bounds);
    this->IsVoxelGridFilterInitialized = true;
  }

  std::array<vtkIdType, 2> pointIdx = { 0, pointcloud->GetNumberOfPoints() - 1 };
  for (const auto& idx : pointIdx)
  {
    double currentTimestamp =
      timestamp->GetTuple1(idx) * this->ConversionFactorToSecond + this->TimeOffset;
    if (!this->Interpolator->IsTimeInRange(currentTimestamp))
    {
      vtkWarningMacro("The trajectory does not have a valid time for the current timestamp, "
        << "interpolation might be invalid.");
    }
  }

  // Transform the points of the pointcloud with the trajectory and add them to the voxel grid
  if (!this->TransformAndAddPoints(timestamp, pointcloud))
  {
    vtkErrorMacro(<< "Aggregation failed.");
    return 0;
  }

  // Get the outputs from the voxel grid and the free points and merge them
  vtkNew<vtkAppendPolyData> appendFilter;
  appendFilter->AddInputData(this->VoxelGrid->GetOutput());
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
int vtkAggregatePointsFromTrajectoryOnline::TransformAndAddPoints(vtkDataArray* timestamp,
  vtkPolyData* pointcloud)
{
  for (vtkIdType i = 0; i < pointcloud->GetNumberOfPoints(); i++)
  {
    // Get the current timestamp in seconds
    double currentTimestamp =
      timestamp->GetTuple1(i) * this->ConversionFactorToSecond + this->TimeOffset;

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

  return 1;
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
void vtkAggregatePointsFromTrajectoryOnline::Clear()
{
  this->VoxelGrid->Clear();
  this->CurrentFrame = 0;
  this->IsVoxelGridFilterInitialized = false;
  this->Initialized = false;
  this->AreBoundsComputed = false;
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
