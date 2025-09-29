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
#include <vtkMatrix4x4.h>
#include <vtkMatrixToHomogeneousTransform.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

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
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return VTK_OK;
  }

  return VTK_ERROR;
}

//-----------------------------------------------------------------------------
bool isNaN(double v)
{
  return v != v;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkPointCloudPinHoleProjector::polyDataToImageData(
  vtkSmartPointer<vtkPolyData> input)
{
  const int width = this->Resolution[0];
  const int height = this->Resolution[1];
  const int numberOfPixels = width * height;

  vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
  imageData->SetOrigin(0, 0, 0);
  imageData->SetSpacing(.1, .1, 1);
  imageData->SetDimensions(width, height, 1);

  vtkPointData* inputPointData = input->GetPointData();
  const int numberOfArrays = inputPointData->GetNumberOfArrays();
  vtkPointData* imagePointData = imageData->GetPointData();

  std::unordered_map<int, int> arrayIdMap;
  int arrayIdMapDeltaCounter = 0;

  // Initialize all scalar values of image
  for (int i = 0; i < numberOfArrays; i++)
  {
    vtkAbstractArray* abstractArray = inputPointData->GetAbstractArray(i);

    if (abstractArray->GetNumberOfComponents() > 1)
    {
      arrayIdMapDeltaCounter += 1;
      continue;
    }

    const int dataType = abstractArray->GetDataType();
    vtkSmartPointer<vtkAbstractArray> da;
    da.TakeReference(vtkAbstractArray::CreateArray(dataType));

    da->SetNumberOfValues(numberOfPixels);
    da->SetName(abstractArray->GetName());
    for (int i = 0; i < numberOfPixels; i++)
    {
      if (this->UseNaN && (dataType == VTK_DOUBLE || dataType == VTK_FLOAT))
      {
        if (dataType == VTK_DOUBLE)
        {
          da->SetVariantValue(i, std::numeric_limits<double>::quiet_NaN());
        }
        else
        {
          da->SetVariantValue(i, std::numeric_limits<float>::quiet_NaN());
        }
      }
      else
      {
        da->SetVariantValue(i, 0);
      }
    }

    imagePointData->AddArray(da);
    arrayIdMap[i - arrayIdMapDeltaCounter] = i;
  }

  // Initialize points info arrays
  vtkSmartPointer<vtkIntArray> pointIdArray = vtkSmartPointer<vtkIntArray>::New();
  vtkSmartPointer<vtkDoubleArray> pointDistanceArray = vtkSmartPointer<vtkDoubleArray>::New();
  pointIdArray->SetName("PointId");
  pointIdArray->SetNumberOfValues(numberOfPixels);
  pointDistanceArray->SetName("PointDistance");
  pointDistanceArray->SetNumberOfValues(numberOfPixels);

  for (int i = 0; i < numberOfPixels; i++)
  {
    pointIdArray->SetValue(i, -1);
    pointDistanceArray->SetValue(i, std::numeric_limits<double>::quiet_NaN());
  }
  imagePointData->AddArray(pointIdArray);
  imagePointData->AddArray(pointDistanceArray);

  // Transform point cloud to align before rasterization
  size_t totalPoints = input->GetNumberOfPoints();
  Eigen::Vector4d eigenPoint;
  Eigen::Matrix3d rollMatrix = Eigen::AngleAxisd(
    -vtkMath::RadiansFromDegrees(this->CameraAngle), this->CameraDirection.normalized())
                                 .toRotationMatrix();
  const Eigen::Matrix3d rot = rollMatrix * this->RotationMatrix;

  auto rotationTransform = vtkSmartPointer<vtkMatrixToHomogeneousTransform>::New();
  auto rotMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  auto rotationMorpher = vtkSmartPointer<vtkTransformPolyDataFilter>::New();

  const double* rd = rot.data();
  double data[16] = {
    rd[0], rd[1], rd[2], 0, rd[3], rd[4], rd[5], 0, rd[6], rd[7], rd[8], 0, 0, 0, 0, 1
  };

  auto translationTransform = vtkSmartPointer<vtkTransform>::New();
  auto translationMorpher = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  translationTransform->Translate(this->GetCameraTranslation().data());
  translationTransform->Inverse();
  translationMorpher->SetTransform(translationTransform);

  rotMatrix->SetData(data);
  rotationTransform->SetInput(rotMatrix);
  rotationMorpher->SetTransform(rotationTransform);

  rotationMorpher->SetInputConnection(translationMorpher->GetOutputPort());

  translationMorpher->SetInputData(input);
  translationMorpher->Update();
  rotationMorpher->Update();

  vtkPolyData* morphed = rotationMorpher->GetOutput();

  // Rasterization
  double point[3];
  double f = this->GetFocalLength();
  const double dWidth = static_cast<double>(width);
  const double dHeight = static_cast<double>(height);
  for (size_t i = 0; i < totalPoints; i++)
  {
    morphed->GetPoint(i, point);
    // If the point is in front of the projection plane...
    if (point[2] > 0)
    {
      int x = point[0] / point[2] * f * dWidth / 2 + dWidth / 2;
      int y = point[1] / point[2] * f * dWidth / 2 + dHeight / 2;

      // ...and its projection is in image bounds
      if (0 <= x && x < width && 0 <= y && y < height)
      {
        int imageIndex = y * width + x;
        double previousDistance = pointDistanceArray->GetValue(imageIndex);
        double currentDistance =
          std::sqrt(std::pow(point[0], 2) + std::pow(point[1], 2) + std::pow(point[2], 2));

        // If two points match the same pixel, we keep the closest one to the camera
        if (previousDistance != previousDistance || currentDistance < previousDistance)
        {
          for (int j = 0; j < numberOfArrays; j++)
          {
            // Copy scalar directly from point cloud
            imagePointData->GetArray(j)->SetTuple(
              imageIndex, i, inputPointData->GetArray(arrayIdMap[j]));
          }

          // Set additional scalars to track projected points
          pointIdArray->SetValue(imageIndex, i);
          pointDistanceArray->SetValue(imageIndex, currentDistance);
        }
      }
    }
  }

  if (this->FillWithNeighbors)
  {
    for (int k = 0; k < imagePointData->GetNumberOfArrays(); k++)
    {
      vtkAbstractArray* arr = imagePointData->GetArray(k);

      // Currently we only fill double arrays, we could use the same logic for float,
      // and use specific values like MAX_INT, MAX_LONG etc... for other types
      if (arr->GetDataType() != VTK_DOUBLE)
      {
        continue;
      }

      vtkDoubleArray* newArray = vtkDoubleArray::SafeDownCast(arr);
      vtkSmartPointer<vtkDoubleArray> oldArray = vtkSmartPointer<vtkDoubleArray>::New();
      oldArray->DeepCopy(newArray);

      int neighborsDx[4] = { -1, 0, 0, 1 };
      int neighborsDy[4] = { 0, -1, 1, 0 };
      // We loop on image pixels
      for (int x = 0; x < width; x++)
      {
        for (int y = 0; y < height; y++)
        {
          int index = y * width + x;
          double val = oldArray->GetValue(index);
          // if the pixel is NaN, we search for its close neighbors
          if (isNaN(val))
          {
            bool neighborFound = false;
            for (int i = 0; i < 4; i++)
            {
              int xn = x + neighborsDx[i];
              int yn = y + neighborsDy[i];
              // neighbor bounds check
              if (0 <= xn && xn < width && 0 <= yn && yn < height)
              {
                int neighborIndex = yn * width + xn;
                double neighborVal = oldArray->GetValue(neighborIndex);
                if (!isNaN(neighborVal))
                {
                  newArray->SetValue(index, neighborVal);
                  neighborFound = true;
                  break;
                }
              }
            }
            // If no neighbor with value if found, set value back to NaN
            if (!neighborFound)
            {
              newArray->SetValue(index, std::numeric_limits<double>::quiet_NaN());
            }
          }
          else
          {
            newArray->SetValue(index, val);
          }
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

  const int width = this->Resolution[0];
  const int height = this->Resolution[1];

  const double r = static_cast<double>(width) / height;
  const double f = this->FocalLength;
  const double m = 3.0;

  double vertices[8][3] = {
    { -r, -1, -m * f },
    { r, -1, -m * f },
    { r, 1, -m * f },
    { -r, 1, -m * f },

    { -m * r, -m, 0 },
    { m * r, -m, 0 },
    { m * r, m, 0 },
    { -m * r, m, 0 },
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
    vtkSmartPointer<vtkIdList> vil = vtkSmartPointer<vtkIdList>::New();
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

  // this->Modified();

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
