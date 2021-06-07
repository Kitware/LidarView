//=========================================================================
//
// Copyright 2020 Kitware, Inc.
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
#include "vtkCameraMapper.h"
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
#include <vtkJPEGReader.h>

#include <boost/filesystem.hpp>

#define POINTS_INPUT_PORT 0
#define TRAJECTORY_INPUT_PORT 1
#define IMAGE_INPUT_PORT 2
#define INPUT_PORT_COUNT 3

#define POINTS_OUTPUT_PORT 0
#define OUTPUT_PORT_COUNT 1
// Use default FillOutputPortInformation as long as OUTPUT_PORT_COUNT is 1

// Implementation of the New function
vtkStandardNewMacro(vtkCameraMapper)

//-----------------------------------------------------------------------------
vtkCameraMapper::vtkCameraMapper()
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//-----------------------------------------------------------------------------
int vtkCameraMapper::FillInputPortInformation(int port, vtkInformation *info)
{
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
  if (port == IMAGE_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData" );
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCameraMapper::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{
  vtkInformation* inImageInfo = inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0);
  if (inImageInfo != nullptr)
  {
    // RequestUpdateExtent is called in the opposite direction (going backward in the pipeline), so we get the requestedTimestamp by looking at the output
    double requestedTimestamp = outputVector->GetInformationObject(POINTS_OUTPUT_PORT)->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    std::vector<double> imageTimesteps = getTimeSteps(inImageInfo);
    int bestImageTimeId = closestElementInOrderedVector(imageTimesteps, requestedTimestamp);
    double bestImageTime = imageTimesteps[bestImageTimeId];

    vtkDebugMacro(<< "vtkCameraMapper::RequestUpdateExtent() with time: "
                  << requestedTimestamp
                  << " closest image time: " << bestImageTime);

    // Position the chosen image in input of the filter, and keep track of its (pipeline) time
    inImageInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), bestImageTime);
    this->CurrentImagePipelineTime = bestImageTime;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCameraMapper::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // Get inputs
  vtkPolyData* pointcloud = vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT], 0);
  vtkPolyData* lidarTrajectory = vtkPolyData::GetData(inputVector[TRAJECTORY_INPUT_PORT], 0);
  vtkImageData* inImg = vtkImageData::GetData(inputVector[IMAGE_INPUT_PORT], 0);

  if (!pointcloud)
  {
    vtkGenericWarningMacro("Null pointer entry, can not launch the filter");
    return 1;
  }

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
        vtkWarningMacro("warning: could not read trajectory from input");
                        return VTK_ERROR;
      }
      this->Trajectory= trajectoryTemp->CreateInterpolator();
      this->Trajectory->SetInterpolationTypeToLinear(); // enough if freq is high TODO: expose this option
      auto unused = vtkSmartPointer<vtkTransform>::New();
      this->Trajectory->InterpolateTransform(0.0, unused); // trigger internal init
    }
  }

  // Get the output
  vtkPolyData* outCloud = vtkPolyData::GetData(outputVector, POINTS_OUTPUT_PORT);
  outCloud->DeepCopy(pointcloud);
  auto pixelCoordsArray = createArray<vtkDoubleArray>("CameraPixelCoordinates", 2, outCloud->GetNumberOfPoints());
  pixelCoordsArray->Fill(-1);
  auto textureCoordsArray = createArray<vtkDoubleArray>("TextureCoordinates", 2, outCloud->GetNumberOfPoints());
  textureCoordsArray->Fill(-1);
  outCloud->GetPointData()->AddArray(pixelCoordsArray);
  outCloud->GetPointData()->SetTCoords(textureCoordsArray);

  vtkDataArray* timestampArray = pointcloud->GetPointData()->GetArray("adjustedtime");
  bool doCorrectPoints = (this->UseTrajectoryToCorrectPoints
                          && (this->Trajectory != nullptr)
                          && (inImg != nullptr)
                          && (timestampArray != nullptr));

  bool doCorrectCameraPose = (this->UseTrajectoryToCompensateCameraMovement
                              && (this->Trajectory != nullptr)
                              && (inImg != nullptr));

  if (this->UseTrajectoryToCorrectPoints && (timestampArray == nullptr))
  {
    vtkWarningMacro("Point cloud does not have an 'adjustedtime' array. Trajectory won't be used to correct points.");
  }

  Eigen::VectorXd W = this->Model.GetParametersVector();

  Eigen::Affine3d cameraCorrection = Eigen::Affine3d::Identity();
  Eigen::Affine3d correction = Eigen::Affine3d::Identity();

  if (doCorrectPoints || doCorrectCameraPose)
  {
    auto poseAtCameraTimeTemp = vtkSmartPointer<vtkTransform>::New();
    this->Trajectory->InterpolateTransform(this->PipelineTimeToLidarTime + this->CurrentImagePipelineTime, poseAtCameraTimeTemp);
    Eigen::Affine3d poseAtCameraTime = ToEigenTransform(poseAtCameraTimeTemp);
    cameraCorrection = poseAtCameraTime.inverse(Eigen::Affine);
    correction = cameraCorrection;
  }

  auto poseAtPointTimeTemp = vtkSmartPointer<vtkTransform>::New(); // place this costly heap allocation outside the loop

  // Project the points in the image
  for (vtkIdType pointIndex = 0; pointIndex < outCloud->GetNumberOfPoints(); ++pointIndex)
  {
    double* pos = outCloud->GetPoint(pointIndex);
    Eigen::Vector3d X(pos[0], pos[1], pos[2]);

    if (doCorrectPoints)
    {
      double pointTimestamp = 1e-6 * timestampArray->GetTuple1(pointIndex);
      this->Trajectory->InterpolateTransform(pointTimestamp, poseAtPointTimeTemp);
      Eigen::Affine3d poseAtPointTime = ToEigenTransform(poseAtPointTimeTemp);

      correction = cameraCorrection * poseAtPointTime;
    }

    Eigen::Vector2d y = this->Model.Projection(correction * X, true);

    double imgWidth = (inImg == nullptr) ? this->ImgSize(0) : static_cast<double>(inImg->GetDimensions()[0]);
    double imgHeight = (inImg == nullptr) ? this->ImgSize(1) : static_cast<double>(inImg->GetDimensions()[1]);

    if ((y(0) >= 0) && (y(0) < imgWidth) &&
        (y(1) >= 0) && (y(1) < imgHeight))
    {
      if (this->UseCameraMask && (this->CameraMask != nullptr))
      {
        double maskValue = this->CameraMask->GetScalarComponentAsDouble(y(0), y(1), 0, 0);
        // skip if invertCameraMask xor mask value is true
        if (this->InvertCameraMask != (maskValue > 0))
        {
          continue;
        }
      }
      pixelCoordsArray->SetTuple2(pointIndex, y(0), y(1));
      textureCoordsArray->SetTuple2(pointIndex, y(0)/imgWidth, y(1)/imgHeight);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkCameraMapper::SetCalibrationFileName(const std::string &argfilename)
{
  this->CalibrationFileName = argfilename;
  int ret = 0;
  if (boost::filesystem::exists(this->CalibrationFileName))
  {
    ret = this->Model.LoadParamsFromFile(this->CalibrationFileName);
  }
  if (!ret)
  {
    vtkWarningMacro("Calibration parameters could not be read from file.");
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCameraMapper::SetCameraMask(const std::string &argfilename)
{
  if (!argfilename.empty())
  {
    vtkSmartPointer<vtkJPEGReader> imgReader = vtkSmartPointer<vtkJPEGReader>::New();
    imgReader->SetFileName(argfilename.c_str());
    imgReader->Update();
    this->CameraMask = imgReader->GetOutput();
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkCameraMapper::Modified()
{
  this->Superclass::Modified();
  this->NeedsToUpdateCachedValues = true; // we do not do the update now, in case multiple Modified() are done in a row
}
