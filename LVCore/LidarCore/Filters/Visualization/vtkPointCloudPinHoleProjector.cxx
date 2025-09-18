/*=========================================================================

  Program: LidarView
  Module:  vtkPointCloudPinHoleProjector.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPointCloudPinHoleProjector.h"

#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <Eigen/Dense>

vtkStandardNewMacro(vtkPointCloudPinHoleProjector);

//-----------------------------------------------------------------------------
vtkPointCloudPinHoleProjector::vtkPointCloudPinHoleProjector()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
int vtkPointCloudPinHoleProjector::FillInputPortInformation(int vtkNotUsed(port),
  vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkPointCloudPinHoleProjector::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return VTK_OK;
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return VTK_OK;
  }

  return VTK_ERROR;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkPointCloudPinHoleProjector::polyDataToImageData(
  vtkSmartPointer<vtkPolyData> input)
{
  const int width = this->Resolution[0];
  const int height = this->Resolution[1];

  vtkNew<vtkImageData> imageData;
  imageData->SetOrigin(0, 0, 0);
  imageData->SetSpacing(.1, .1, 1);
  imageData->SetDimensions(width, height, 1);

  vtkPointData* inputPointData = input->GetPointData();
  const int numberOfArrays = inputPointData->GetNumberOfArrays();
  vtkPointData* imagePointData = imageData->GetPointData();

  // Initialize all scalar values of image
  for (int i = 0; i < numberOfArrays; i++)
  {
    vtkAbstractArray* abstractArray = inputPointData->GetAbstractArray(i);
    const int dataType = abstractArray->GetDataType();

    vtkDataArray* da = vtkDataArray::CreateDataArray(dataType);

    da->SetName(abstractArray->GetName());
    for (int x = 0; x < width; x++)
    {
      for (int y = 0; y < height; y++)
      {
        if (this->UseNaN && dataType == VTK_DOUBLE)
        {
          da->InsertNextTuple1(std::numeric_limits<double>::quiet_NaN());
        }
        else
        {
          da->InsertNextTuple1(0);
        }
      }
    }

    imagePointData->AddArray(da);
  }

  // Initialize points info arrays
  vtkNew<vtkIntArray> pointIdArray;
  vtkNew<vtkDoubleArray> pointDistanceArray;
  pointIdArray->SetName("PointId");
  pointDistanceArray->SetName("PointDistance");

  for (int i = 0; i < width * height; i++)
  {
    pointIdArray->InsertNextValue(-1);
    pointDistanceArray->InsertNextValue(std::numeric_limits<double>::quiet_NaN());
  }
  imagePointData->AddArray(pointIdArray);
  imagePointData->AddArray(pointDistanceArray);

  // Rasterization
  size_t totalPoints = input->GetNumberOfPoints();
  double point[3];
  Eigen::Vector4d eigenPoint;
  Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();
  Eigen::Matrix3d rollMatrix = Eigen::AngleAxisd(
    -vtkMath::RadiansFromDegrees(this->CameraAngle), this->CameraDirection.normalized())
                                 .toRotationMatrix();
  transform.block<3, 3>(0, 0) = rollMatrix * this->RotationMatrix;
  transform.block<3, 1>(0, 3) = this->CameraTranslation;

  Eigen::Matrix4d projector = transform.inverse();
  for (size_t i = 0; i < totalPoints; i++)
  {
    input->GetPoint(i, point);
    eigenPoint << point[0], point[1], point[2], 1.0;
    eigenPoint = projector * eigenPoint;
    Eigen::Vector3d transformedPoint = eigenPoint.head<3>();

    // If the point is in front of the projection plane...
    if (transformedPoint.z() > 0)
    {
      int x = transformedPoint.x() / transformedPoint.z() * width / 2 + width / 2;
      int y = transformedPoint.y() / transformedPoint.z() * width / 2 + height / 2;

      int imageIndex = y * width + x;
      double previousDistance = pointDistanceArray->GetValue(imageIndex);
      double currentDistance = transformedPoint.norm();

      // ...and its projection is in image bounds
      if (0 <= x && x < width && 0 <= y && y < height)
      {
        // If two points match the same pixel, we keep the closest one to the camera
        if (previousDistance != previousDistance || currentDistance < previousDistance)
        {
          for (int j = 0; j < numberOfArrays; j++)
          {
            // Copy scalar directly from
            imagePointData->GetArray(j)->SetTuple(imageIndex, i, inputPointData->GetArray(j));
          }

          // Set additional scalars to track projected points
          pointIdArray->SetValue(imageIndex, i);
          pointDistanceArray->SetValue(imageIndex, currentDistance);
        }
      }
    }
  }

  return imageData;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkPointCloudPinHoleProjector::generateCameraModel()
{
  vtkSmartPointer<vtkPolyData> cameraModel = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();

  double vertices[8][3] = {
    { -1.6, -1, -1 },
    { 1.6, -1, -1 },
    { 1.6, 1, -1 },
    { -1.6, 1, -1 },

    { -3, -3, 4 },
    { 3, -3, 4 },
    { 3, 3, 4 },
    { -3, 3, 4 },
  };

  int faces[6][4] = {
    { 0, 1, 2, 3 },
    { 4, 5, 6, 7 },
    { 0, 3, 7, 4 },
    { 1, 2, 6, 5 },
    { 0, 1, 5, 4 },
    { 2, 3, 7, 6 },
  };

  Eigen::Vector4d eigenPoint;
  Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();
  Eigen::Matrix3d rollMatrix = Eigen::AngleAxisd(
    vtkMath::RadiansFromDegrees(this->CameraAngle), this->CameraDirection.normalized())
                                 .toRotationMatrix();
  transform.block<3, 3>(0, 0) = rollMatrix * this->RotationMatrix;
  transform.block<3, 1>(0, 3) = this->CameraTranslation;

  for (size_t i = 0; i < 8; i++)
  {
    double* vertice = vertices[i];
    eigenPoint << vertice[0], vertice[1], vertice[2], 1.0;
    eigenPoint = transform * eigenPoint;
    Eigen::Vector3d transformedPoint = eigenPoint.head<3>();
    points->InsertPoint(i, transformedPoint.x(), transformedPoint.y(), transformedPoint.z());
  }

  for (size_t i = 0; i < 6; i++)
  {
    int* face = faces[i];
    vtkNew<vtkIdList> vil;
    for (size_t j = 0; j < 4; j++)
      vil->InsertNextId(face[j]);
    polys->InsertNextCell(vil);
  }

  cameraModel->SetPoints(points);
  cameraModel->SetPolys(polys);

  return cameraModel;
}

//-----------------------------------------------------------------------------
int vtkPointCloudPinHoleProjector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkImageData* output = vtkImageData::GetData(outputVector, 0);
  vtkPolyData* modelOutput = vtkPolyData::GetData(outputVector, 1);

  vtkSmartPointer<vtkImageData> imageData = polyDataToImageData(input);
  output->ShallowCopy(imageData);

  vtkSmartPointer<vtkPolyData> cameraModel = this->generateCameraModel();
  modelOutput->DeepCopy(cameraModel);

  this->Modified();

  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkPointCloudPinHoleProjector::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    0,
    this->Resolution[0] - 1,
    0,
    this->Resolution[1] - 1,
    0,
    0);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, 1);

  return VTK_OK;
}

//-----------------------------------------------------------------------------
void vtkPointCloudPinHoleProjector::SetCameraTranslation(double x, double y, double z)
{
  this->CameraTranslation[0] = x;
  this->CameraTranslation[1] = y;
  this->CameraTranslation[2] = z;
  this->UpdateTransform();
}

//-----------------------------------------------------------------------------
Eigen::Vector3d vtkPointCloudPinHoleProjector::GetCameraTranslation()
{
  return this->CameraTranslation;
}

//-----------------------------------------------------------------------------
void vtkPointCloudPinHoleProjector::SetCameraDirection(double x, double y, double z)
{
  this->CameraDirection[0] = x;
  this->CameraDirection[1] = y;
  this->CameraDirection[2] = z;
  this->CameraDirection.normalize();
  this->UpdateTransform();
}

//-----------------------------------------------------------------------------
Eigen::Vector3d vtkPointCloudPinHoleProjector::GetCameraDirection()
{
  return this->CameraDirection;
}

//-----------------------------------------------------------------------------
void vtkPointCloudPinHoleProjector::SetCameraAngle(double angle)
{
  this->CameraAngle = angle;
  this->UpdateTransform();
}

//-----------------------------------------------------------------------------
double vtkPointCloudPinHoleProjector::GetCameraAngle()
{
  return this->CameraAngle;
}

//-----------------------------------------------------------------------------
void vtkPointCloudPinHoleProjector::UpdateTransform()
{
  Eigen::Vector3d up(0, 0, 1);
  Eigen::Vector3d xAxis = this->CameraDirection.cross(up);
  xAxis.normalize();

  Eigen::Vector3d yAxis = xAxis.cross(this->CameraDirection);
  yAxis.normalize();

  Eigen::Vector3d direction(this->CameraDirection);

  this->RotationMatrix << xAxis.x(), yAxis.x(), direction.x(), xAxis.y(), yAxis.y(), direction.y(),
    xAxis.z(), yAxis.z(), direction.z();

  this->Modified();
}
