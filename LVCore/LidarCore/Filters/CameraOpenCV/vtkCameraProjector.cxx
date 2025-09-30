/*=========================================================================

  Program:   LidarView
  Module:    vtkCameraProjector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// LOCAL
#include "vtkCameraProjector.h"
#include "CameraProjection.h"
#include "vtkEigenTools.h"
#include "vtkHelper.h"
#include "vtkOpenCVVideoReader.h"
#include "vtkPipelineTools.h"
#include "vtkTemporalTransforms.h"

// VTK
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSMPTools.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransform.h>

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
int vtkCameraProjector::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == IMAGE_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  if (port == POINTS_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == TRAJECTORY_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkCameraProjector::FillOutputPortInformation(int port, vtkInformation* info)
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

  if (pointTimesteps.size() == 0)
  {
    vtkErrorMacro("No timesteps available in the input point cloud");
    return 0;
  }

  double timeRange[2] = { pointTimesteps[0], pointTimesteps[pointTimesteps.size() - 1] };
  // We provide the same timestamps for all outputs
  for (int i = 0; i < OUTPUT_PORT_COUNT; i++)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(i);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      &pointTimesteps.front(),
      pointTimesteps.size());

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
  // RequestUpdateExtent is called in the opposite direction (going backward in the pipeline), so we
  // get the requestedTimestamp by looking at the output
  double requestedTimestampPoints = outputVector->GetInformationObject(POINTS_OUTPUT_PORT)
                                      ->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  double requestedTimestampImage = outputVector->GetInformationObject(IMAGE_WITH_POINTS_OUTPUT_PORT)
                                     ->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  double requestedTimestampProjectedPoints =
    outputVector->GetInformationObject(PROJECTED_POINTS_OUTPUT_PORT)
      ->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  this->FrameTimestamp = requestedTimestampPoints;

  // I do not fully understand why these three times are differents (even
  // though the same timesteps are copied to all outputs by RequestInformation
  // and all outputs are all shown), so we take the maximum.
  // This does not risk introducing a bug because we can use any image as long
  // as there is not too many occlusions and we know its instant of capture.
  double requestedTimestamp = std::max(
    requestedTimestampPoints, std::max(requestedTimestampImage, requestedTimestampProjectedPoints));

  // If there is no image, nothing must be done, the video path will be used instead
  if (vtkImageData::GetData(inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0)) == nullptr)
  {
    return 1;
  }
  std::vector<double> imageTimesteps =
    getTimeSteps(inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0));
  int bestImageTimeId = closestElementInOrderedVector(imageTimesteps, requestedTimestamp);
  double bestImageTime = imageTimesteps[bestImageTimeId];

  vtkDebugMacro(<< "vtkCameraProjector::RequestUpdateExtent() with time: " << requestedTimestamp
                << " closest image time: " << bestImageTime);

  // Position the chosen image in input of the filter, and keep track of its (pipeline) time
  inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), bestImageTime);
  this->CurrentImagePipelineTime = bestImageTime;

  return 1;
}

//-----------------------------------------------------------------------------
int vtkCameraProjector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->UseCalibrationFile)
  {
    this->SetCameraModelParams();
    this->ModelValid = true;
  }

  if (!this->ModelValid)
  {
    vtkErrorMacro("Camera model is not valid");
    return 0;
  }

  // Get inputs
  vtkImageData* inImg =
    vtkImageData::GetData(inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0));
  vtkPolyData* pointcloud =
    vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT]->GetInformationObject(0));
  vtkPolyData* lidarTrajectory =
    vtkPolyData::GetData(inputVector[TRAJECTORY_INPUT_PORT]->GetInformationObject(0));

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
        return 0;
      }
      this->Trajectory = trajectoryTemp->CreateInterpolator();
      this->Trajectory
        ->SetInterpolationTypeToLinear(); // enough if freq is high TODO: expose this option
      auto unused = vtkSmartPointer<vtkTransform>::New();
      this->Trajectory->InterpolateTransform(0.0, unused); // trigger internal init
    }
  }

  // Get the output
  vtkImageData* outImg =
    vtkImageData::GetData(outputVector->GetInformationObject(IMAGE_WITH_POINTS_OUTPUT_PORT));
  vtkPolyData* outCloud =
    vtkPolyData::GetData(outputVector->GetInformationObject(POINTS_OUTPUT_PORT));
  vtkPolyData* projectedCloud =
    vtkPolyData::GetData(outputVector->GetInformationObject(PROJECTED_POINTS_OUTPUT_PORT));

  vtkNew<vtkImageData> inputImg;

  if (inImg == nullptr)
  {
    vtkNew<vtkOpenCVVideoReader> reader;
    reader->SetTimeOffset(this->VideoTimeOffset);
    reader->SetFileName(this->VideoPath.c_str());
    reader->SetForceTimeStamp(true);
    reader->SetForcedTimeStamp(this->FrameTimestamp);
    reader->Update();
    inputImg->DeepCopy(reader->GetOutput(0));
    inImg = inputImg;
    outImg->DeepCopy(inputImg);
  }

  if (!inImg || !pointcloud)
  {
    vtkGenericWarningMacro("Null pointer entry, can not launch the filter");
    return 1;
  }

  vtkNew<vtkPoints> outPoints;
  vtkNew<vtkCellArray> outVerts;
  outPoints->Resize(pointcloud->GetNumberOfPoints());
  outVerts->SetNumberOfCells(pointcloud->GetNumberOfPoints());
  outCloud->SetPoints(outPoints);
  outCloud->SetVerts(outVerts);
  outCloud->GetPointData()->CopyAllocate(pointcloud->GetPointData());

  outImg->DeepCopy(inImg);

  // projectedCloud->DeepCopy(outCloud);
  vtkNew<vtkPoints> outPointsProjected;
  vtkNew<vtkCellArray> outVertsProjected;
  outPointsProjected->Resize(pointcloud->GetNumberOfPoints());
  outVertsProjected->SetNumberOfCells(pointcloud->GetNumberOfPoints());
  projectedCloud->SetPoints(outPointsProjected);
  projectedCloud->SetVerts(outVertsProjected);
  projectedCloud->GetPointData()->CopyAllocate(pointcloud->GetPointData());

  // Store original indices of the points in the projected cloud for potential matching (eg. for
  // reprojections)
  auto preProjectionIndexArray =
    createArray<vtkIntArray>("preProjectionIndex", 1, projectedCloud->GetNumberOfPoints());
  projectedCloud->GetPointData()->AddArray(preProjectionIndexArray);

  vtkDataArray* intensity = pointcloud->GetPointData()->GetArray("intensity");
  vtkDataArray* timestampArray = pointcloud->GetPointData()->GetArray("adjustedtime");

  // Get RGB array from input pointcloud
  vtkSmartPointer<vtkIntArray> inputRGBArray =
    vtkIntArray::SafeDownCast(outCloud->GetPointData()->GetArray(this->ColorArrayName.c_str()));

  // Try to get RGB array, if it does not exist, create it and fill it
  vtkNew<vtkIntArray> rgbArray;
  rgbArray->SetNumberOfComponents(3);
  rgbArray->SetNumberOfTuples(pointcloud->GetNumberOfPoints());
  rgbArray->SetName(this->ColorArrayName.c_str());

  vtkNew<vtkIntArray> rgbArrayProjected;
  rgbArrayProjected->SetNumberOfComponents(3);
  rgbArrayProjected->SetNumberOfTuples(pointcloud->GetNumberOfPoints());
  rgbArrayProjected->SetName(this->ColorArrayName.c_str());

  Eigen::VectorXd W = this->Model.GetParametersVector();
  auto poseAtCameraTimeTemp = vtkSmartPointer<vtkTransform>::New();

  Eigen::Transform<double, 3, Eigen::Affine> poseAtCameraTime;
  if (this->UseTrajectoryToCorrectPoints && this->Trajectory != nullptr)
  {
    double pointTimestamp = 1e-6 * timestampArray->GetTuple1(0);
    this->Trajectory->InterpolateTransform(pointTimestamp, poseAtCameraTimeTemp);
    poseAtCameraTimeTemp->Update();
    poseAtCameraTime = ToEigenTransform(poseAtCameraTimeTemp);
  }
  // place this costly heap allocation outside the loop
  auto poseAtPointTimeTemp = vtkSmartPointer<vtkTransform>::New();

  // Project the points in the image
  vtkIdType numPoints = 0;
  vtkIdType numPointsProjected = 0;
  for (vtkIdType pointIndex = 0; pointIndex < pointcloud->GetNumberOfPoints(); ++pointIndex)
  {
    double pos[3];
    pointcloud->GetPoint(pointIndex, pos);
    Eigen::Vector3d X(pos[0], pos[1], pos[2]);
    Eigen::Vector2d y;
    if (this->UseTrajectoryToCorrectPoints && this->Trajectory != nullptr)
    {
      double pointTimestamp = 1e-6 * timestampArray->GetTuple1(pointIndex);
      this->Trajectory->InterpolateTransform(pointTimestamp, poseAtPointTimeTemp);
      poseAtPointTimeTemp->Update();

      Eigen::Transform<double, 3, Eigen::Affine> poseAtPointTime =
        ToEigenTransform(poseAtPointTimeTemp);

      Eigen::Transform<double, 3, Eigen::Affine> correction =
        poseAtCameraTime.inverse(Eigen::Affine) * poseAtPointTime;

      Eigen::Vector3d Xcorrected = correction.rotation() * X + correction.translation();
      X = Xcorrected;
    }

    y = this->Model.Projection(X, true);

    // y represents the pixel coordinates using opencv convention, we need to
    // go back to vtkImageData pixel convention
    int vtkRow = static_cast<int>(y(1));
    int vtkCol = static_cast<int>(y(0));

    bool isInside = (vtkRow >= 0) && (vtkRow < inImg->GetDimensions()[1]) && (vtkCol >= 0) &&
      (vtkCol < inImg->GetDimensions()[0]);

    if (isInside || !this->ColorizedOutputOnly)
    {
      outCloud->GetPoints()->InsertNextPoint(pos);
      outCloud->GetVerts()->InsertNextCell(1, &numPoints);
      outCloud->GetPointData()->CopyData(pointcloud->GetPointData(), pointIndex, numPoints);
      numPoints++;
    }

    if (!isInside)
    {
      if (!this->ColorizedOutputOnly)
      {
        if (inputRGBArray)
        {
          double* rgb = inputRGBArray->GetTuple3(pointIndex);
          rgbArray->SetTuple3(numPoints - 1, rgb[0], rgb[1], rgb[2]);
        }
        else
        {
          rgbArray->SetTuple3(numPoints - 1, 255, 255, 255);
        }
      }
      continue;
    }

    vtkRow = std::min(std::max(0, vtkRow), inImg->GetDimensions()[1] - 1);
    vtkCol = std::min(std::max(0, vtkCol), inImg->GetDimensions()[0] - 1);

    // register the point if it is valid
    double pt[3] = { y(0), y(1), 0. };
    projectedCloud->GetPoints()->InsertNextPoint(pt);
    projectedCloud->GetVerts()->InsertNextCell(1, &numPointsProjected);
    projectedCloud->GetPointData()->CopyData(
      pointcloud->GetPointData(), pointIndex, numPointsProjected);
    numPointsProjected++;

    preProjectionIndexArray->InsertNextTuple1(pointIndex);

    // Get its color
    double intensityValue = intensity->GetTuple1(pointIndex);
    Eigen::Vector3d color = GetRGBColourFromReflectivity(intensityValue, 0, 255);

    double rgb[3];
    for (int k = 0; k < 3; ++k)
    {
      for (int colOffset = -(this->ProjectedPointSizeInImage / 2);
           colOffset < ((this->ProjectedPointSizeInImage + 1) / 2);
           colOffset++)
      {
        for (int rowOffset = -(this->ProjectedPointSizeInImage / 2);
             rowOffset < ((this->ProjectedPointSizeInImage + 1) / 2);
             rowOffset++)
        {
          int c = std::min(std::max(0, vtkCol + colOffset), inImg->GetDimensions()[0] - 1);
          int r = std::min(std::max(0, vtkRow + rowOffset), inImg->GetDimensions()[1] - 1);
          outImg->SetScalarComponentFromDouble(c, r, 0, k, color(2 - k));
        }
      }
      rgb[k] = inImg->GetScalarComponentAsDouble(vtkCol, vtkRow, 0, k);
    }
    rgbArray->SetTuple3(numPoints - 1, rgb[0], rgb[1], rgb[2]);
    rgbArrayProjected->SetTuple3(numPointsProjected - 1, rgb[0], rgb[1], rgb[2]);
  }

  // Resize data
  outCloud->GetPoints()->Resize(numPoints);
  outCloud->GetVerts()->SetNumberOfCells(numPoints);

  rgbArray->SetNumberOfTuples(numPoints);
  outCloud->GetPointData()->AddArray(rgbArray);
  outCloud->GetPointData()->SetActiveScalars(rgbArray->GetName());

  projectedCloud->GetPoints()->Resize(numPointsProjected);
  projectedCloud->GetVerts()->SetNumberOfCells(numPointsProjected);

  rgbArrayProjected->Resize(numPointsProjected);
  projectedCloud->GetPointData()->AddArray(rgbArrayProjected);
  projectedCloud->GetPointData()->SetActiveScalars(rgbArrayProjected->GetName());

  return 1;
}

//------------------------------------------------------------------------------
void vtkCameraProjector::SetUseCalibrationFile(bool argUseCalibrationFile)
{
  this->UseCalibrationFile = argUseCalibrationFile;
  if (this->UseCalibrationFile)
  {
    this->SetFileName(this->Filename);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCameraProjector::SetFileName(const std::string& argfilename)
{
  if (!this->UseCalibrationFile)
  {
    return;
  }
  this->Filename = argfilename;
  int ret = this->Model.LoadParamsFromFile(this->Filename);
  if (!ret)
  {
    vtkWarningMacro("Calibration parameters could not be read from file.");
    this->ModelValid = false;
    return;
  }
  this->ModelValid = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::Modified()
{
  this->Superclass::Modified();
  // do not update now, in case multiple Modified() are done in a row
  this->NeedsToUpdateCachedValues = true;
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetCameraType(int type)
{
  this->Type = static_cast<ProjectionType>(type);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetRotation(double roll, double pitch, double yaw)
{
  Eigen::Matrix3d r = RollPitchYawToMatrix(roll, pitch, yaw);
  this->R = r;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetTranslation(double x, double y, double z)
{
  Eigen::Vector3d t = { x, y, z };
  this->T = t;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetFocal(double fx, double fy)
{
  this->K(0, 0) = fx;
  this->K(1, 1) = fy;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetOpticalCenter(double cx, double cy)
{
  this->K(0, 2) = cx;
  this->K(1, 2) = cy;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetSkew(double skew)
{
  this->K(0, 1) = skew;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetKCoeffs(double k1, double k2)
{
  this->Optics(0) = k1;
  this->Optics(1) = k2;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetPCoeffs(double p1, double p2, double p3, double p4)
{
  this->Optics(2) = p1;
  this->Optics(3) = p2;
  this->Optics(4) = p3;
  this->Optics(5) = p4;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraProjector::SetCameraModelParams()
{
  this->Model.SetR(this->R);
  this->Model.SetT(this->T);
  this->Model.SetK(this->K);
  this->Model.SetOptics(this->Optics);
  this->Model.SetCameraModelType(this->Type);
}
