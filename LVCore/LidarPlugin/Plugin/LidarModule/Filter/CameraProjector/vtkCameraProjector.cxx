//=========================================================================
//
// Copyright 2019 Kitware, Inc.
// Author: Guilbert Pierre (spguilbert@gmail.com)
// Data: 03-27-2019
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

// LOCAL
#include "vtkCameraProjector.h"
#include "vtkTemporalTransforms.h"
#include "CameraProjection.h"
#include "vtkHelper.h"
#include "vtkEigenTools.h"
#include "vtkPipelineTools.h"

// VTK
#include <vtkImageData.h>
#include <vtkDataArray.h>
#include <vtkIntArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#define IMAGE_INPUT_PORT 0
#define POINTS_INPUT_PORT 1
#define TRAJECTORY_INPUT_PORT 2
#define INPUT_PORT_COUNT 3

#define IMAGE_WITH_POINTS_OUTPUT_PORT 0
#define POINTS_OUTPUT_PORT 1 // this clearly is the most useful output (others are cosmetic/debug)
#define PROJECTED_POINTS_OUTPUT_PORT 2
#define OUTPUT_PORT_COUNT 3

// Implementation of the New function
vtkStandardNewMacro(vtkCameraProjector)

//-----------------------------------------------------------------------------
vtkCameraProjector::vtkCameraProjector()
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//-----------------------------------------------------------------------------
int vtkCameraProjector::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == IMAGE_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData" );
    return 1;
  }
  if (port == POINTS_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }
  if (port == TRAJECTORY_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkCameraProjector::FillOutputPortInformation(int port, vtkInformation *info)
{
  if (port == IMAGE_WITH_POINTS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }
  if (port == POINTS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == PROJECTED_POINTS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkCameraProjector::RequestInformation(vtkInformation* vtkNotUsed(request),
                       vtkInformationVector** inputVector,
                       vtkInformationVector* outputVector)
{
  // Propagate the information that steps are available.
  // We choose to propagate the timestep from the point cloud, because this is
  // the main output of this filter.
  vtkInformation* inPointsInfo = inputVector[POINTS_INPUT_PORT]->GetInformationObject(0);
  std::vector<double> pointTimesteps = getTimeSteps(inPointsInfo);

  double timeRange[2] = {pointTimesteps[0], pointTimesteps[pointTimesteps.size() - 1]};

  // We provide the same timestamps for all outputs
  for (int i = 0; i < OUTPUT_PORT_COUNT; i++)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(i);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &pointTimesteps.front(), pointTimesteps.size());

    // Is this needed ? I think not
    // indicate that this filter produces continuous timestep
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCameraProjector::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{
  // RequestUpdateExtent is called in the opposite direction (going backward in the pipeline), so we get the requestedTimestamp by looking at the output
  double requestedTimestampPoints = outputVector->GetInformationObject(POINTS_OUTPUT_PORT)->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  double requestedTimestampImage = outputVector->GetInformationObject(IMAGE_WITH_POINTS_OUTPUT_PORT)->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  double requestedTimestampProjectedPoints = outputVector->GetInformationObject(PROJECTED_POINTS_OUTPUT_PORT)->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  // I do not fully understand why these three times are differents (even
  // though the same timesteps are copied to all outputs by RequestInformation
  // and all outputs are all shown), so we take the maximum.
  // This does not risk introducing a bug because we can use any image as long
  // as there is not too many occlusions and we know its instant of capture.
  double requestedTimestamp = std::max(requestedTimestampPoints, std::max(requestedTimestampImage, requestedTimestampProjectedPoints));

  std::vector<double> imageTimesteps = getTimeSteps(inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0));
  int bestImageTimeId = closestElementInOrderedVector(imageTimesteps, requestedTimestamp);
  double bestImageTime = imageTimesteps[bestImageTimeId];

  vtkDebugMacro(<< "vtkCameraProjector::RequestUpdateExtent() with time: "
                << requestedTimestamp
                << " closest image time: " << bestImageTime);

  // Position the chosen image in input of the filter, and keep track of its (pipeline) time
  inputVector[IMAGE_INPUT_PORT]
      ->GetInformationObject(0)
      ->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), bestImageTime);
  this->CurrentImagePipelineTime = bestImageTime;

  return 1;
}

//-----------------------------------------------------------------------------
int vtkCameraProjector::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // Get inputs
  vtkImageData *inImg = vtkImageData::GetData(inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0));
  vtkPolyData *pointcloud = vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT]->GetInformationObject(0));
  vtkPolyData* lidarTrajectory = vtkPolyData::GetData(inputVector[TRAJECTORY_INPUT_PORT]->GetInformationObject(0));

  if (this->NeedsToUpdateCachedValues)
  {
    this->NeedsToUpdateCachedValues = false;
    // The lidarTrajectory is optional
    this->Trajectory = nullptr;
    if (lidarTrajectory != nullptr)
    {
      auto trajectoryTemp = vtkTemporalTransforms::CreateFromPolyData(lidarTrajectory);
      if (trajectoryTemp == nullptr)
      {
        vtkWarningMacro("warning: could not read trajectory from input 1");
                        return VTK_ERROR;
      }
      this->Trajectory= trajectoryTemp->CreateInterpolator();
      this->Trajectory->SetInterpolationTypeToLinear(); // enough if freq is high TODO: expose this option
      auto unused = vtkSmartPointer<vtkTransform>::New();
      this->Trajectory->InterpolateTransform(0.0, unused); // trigger internal init
    }
  }


  // Get the output
  vtkImageData* outImg = vtkImageData::GetData(outputVector->GetInformationObject(IMAGE_WITH_POINTS_OUTPUT_PORT));
  vtkPolyData* outCloud = vtkPolyData::GetData(outputVector->GetInformationObject(POINTS_OUTPUT_PORT));
  vtkPolyData* projectedCloud = vtkPolyData::GetData(outputVector->GetInformationObject(PROJECTED_POINTS_OUTPUT_PORT));
  if (!inImg || !pointcloud)
  {
    vtkGenericWarningMacro("Null pointer entry, can not launch the filter");
    return 1;
  }

  outImg->DeepCopy(inImg);
  outCloud->DeepCopy(pointcloud);
  projectedCloud->DeepCopy(pointcloud);
  projectedCloud->GetPoints()->Resize(0);
  for (int i = 0; i < projectedCloud->GetPointData()->GetNumberOfArrays(); ++i)
  {
    projectedCloud->GetPointData()->GetArray(i)->Resize(0);
  }

  // Store original indices of the points in the projected cloud for potential matching (eg. for reprojections)
  auto preProjectionIndexArray = createArray<vtkIntArray>("preProjectionIndex", 1, projectedCloud->GetNumberOfPoints());
  projectedCloud->GetPointData()->AddArray(preProjectionIndexArray);

  vtkDataArray* intensity = outCloud->GetPointData()->GetArray("intensity");
  vtkDataArray* timestampArray = pointcloud->GetPointData()->GetArray("adjustedtime");

  // Try to get RGB array, if it does not exist, create it and fill it
  vtkSmartPointer<vtkIntArray> rgbArray = vtkIntArray::SafeDownCast(outCloud->GetPointData()->GetArray(this->ColorArrayName.c_str()));
  if (!rgbArray)
  {
    rgbArray = createArray<vtkIntArray>(std::string(this->ColorArrayName), 3, outCloud->GetNumberOfPoints());
    outCloud->GetPointData()->AddArray(rgbArray);

    // fill tuples to (255, 255, 255)
    rgbArray->Fill(255);
  }

  Eigen::VectorXd W = this->Model.GetParametersVector();

  Eigen::Transform<double, 3, Eigen::Affine> poseAtCameraTime;
  if (this->UseTrajectoryToCorrectPoints && this->Trajectory != nullptr)
  {
    auto poseAtCameraTimeTemp = vtkSmartPointer<vtkTransform>::New();
    this->Trajectory->InterpolateTransform(this->PipelineTimeToLidarTime + this->CurrentImagePipelineTime, poseAtCameraTimeTemp);
    poseAtCameraTime = ToEigenTransform(poseAtCameraTimeTemp);
  }
  auto poseAtPointTimeTemp = vtkSmartPointer<vtkTransform>::New(); // place this costly heap allocation outside the loop

  // Project the points in the image
  for (int pointIndex = 0; pointIndex < outCloud->GetNumberOfPoints(); ++pointIndex)
  {
    double* pos = outCloud->GetPoint(pointIndex);
    Eigen::Vector3d X(pos[0], pos[1], pos[2]);
    Eigen::Vector2d y;
    if (this->UseTrajectoryToCorrectPoints && this->Trajectory != nullptr)
    {
      double pointTimestamp = 1e-6 * timestampArray->GetTuple1(pointIndex);
      this->Trajectory->InterpolateTransform(pointTimestamp, poseAtPointTimeTemp);
      Eigen::Transform<double, 3, Eigen::Affine> poseAtPointTime = ToEigenTransform(poseAtPointTimeTemp);

      Eigen::Transform<double, 3, Eigen::Affine> correction = poseAtCameraTime.inverse(Eigen::Affine) * poseAtPointTime;

      Eigen::Vector3d Xcorrected = correction.rotation() * X + correction.translation();
      X = Xcorrected;
    }

    y = this->Model.Projection(X, true);

    // y represents the pixel coordinates using opencv convention, we need to
    // go back to vtkImageData pixel convention
    int vtkRow = static_cast<int>(y(1));
    int vtkCol = static_cast<int>(y(0));

    if ((vtkRow < 0) || (vtkRow >= inImg->GetDimensions()[1]) ||
        (vtkCol < 0) || (vtkCol >= inImg->GetDimensions()[0]))
    {
      continue;
    }

    vtkRow = std::min(std::max(0, vtkRow), inImg->GetDimensions()[1] - 1);
    vtkCol = std::min(std::max(0, vtkCol), inImg->GetDimensions()[0] - 1);

    // register the point if it is valid
    double pt[3] = {y(0), y(1), 0};
    projectedCloud->GetPoints()->InsertNextPoint(pt);
    for (int i = 0; i < pointcloud->GetPointData()->GetNumberOfArrays(); ++i)
    {
      projectedCloud->GetPointData()->GetArray(i)->InsertNextTuple(
            pointcloud->GetPointData()->GetArray(i)->GetTuple(pointIndex));
    }
    preProjectionIndexArray->InsertNextTuple1(pointIndex);

    // Get its color
    double intensityValue = intensity->GetTuple1(pointIndex);
    Eigen::Vector3d color = GetRGBColourFromReflectivity(intensityValue, 0, 255);

    double rgb[3];
    for (int k = 0; k < 3; ++k)
    {
      for (int colOffset = - (this->ProjectedPointSizeInImage / 2); colOffset < ((this->ProjectedPointSizeInImage + 1) / 2); colOffset ++)
      {
        for (int rowOffset = - (this->ProjectedPointSizeInImage / 2); rowOffset < ((this->ProjectedPointSizeInImage + 1) / 2); rowOffset ++)
        {
          int c = std::min(std::max(0, vtkCol + colOffset), inImg->GetDimensions()[0] - 1);
          int r = std::min(std::max(0, vtkRow + rowOffset), inImg->GetDimensions()[1] - 1);
          outImg->SetScalarComponentFromDouble(c, r, 0, k, color(2 - k));
        }
      }
      rgb[k] = inImg->GetScalarComponentAsDouble(vtkCol, vtkRow, 0, k);
    }
    rgbArray->SetTuple3(pointIndex, rgb[0], rgb[1], rgb[2]);
  }

  vtkNew<vtkIdTypeArray> cells;
  cells->SetNumberOfValues(projectedCloud->GetNumberOfPoints() * 2);
  vtkIdType* ids = cells->GetPointer(0);
  for (vtkIdType i = 0; i < projectedCloud->GetNumberOfPoints(); ++i)
  {
    ids[i * 2] = 1;
    ids[i * 2 + 1] = i;
  }
  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->SetCells(projectedCloud->GetNumberOfPoints(), cells.GetPointer());
  projectedCloud->SetVerts(cellArray);

  return 1;
}

//------------------------------------------------------------------------------
void vtkCameraProjector::SetFileName(const std::string &argfilename)
{
  this->Filename = argfilename;
  int ret = this->Model.LoadParamsFromFile(this->Filename);
  if (!ret)
  {
    vtkWarningMacro("Calibration parameters could not be read from file.");
  }
  this->Modified();
}


//-----------------------------------------------------------------------------
void vtkCameraProjector::Modified()
{
  this->Superclass::Modified();
  this->NeedsToUpdateCachedValues = true; // we do not do the update now, in case multiple Modified() are done in a row
}

