/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBasedClustering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// LOCAL
#include "vtkImageBasedClustering.h"

// VTK
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkImageDilateErode3D.h>
#include <vtkImageThreshold.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSMPTools.h>
#include <vtkStaticPointLocator.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include "vtkImagesOperations.h"
#include "vtkPointCloudPinHoleProjector.h"

#include <queue>

using cluster_map = std::unordered_map<int, std::vector<std::pair<int, int>>>;

// Implementation of the New function
vtkStandardNewMacro(vtkImageBasedClustering)

//-----------------------------------------------------------------------------
vtkImageBasedClustering::vtkImageBasedClustering()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(3);
}

//------------------------------------------------------------------------------
int vtkImageBasedClustering::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
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
int vtkImageBasedClustering::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return VTK_OK;
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return VTK_OK;
  }
  else if (port == 2)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return VTK_OK;
  }

  return VTK_ERROR;
}

//------------------------------------------------------------------------------
void appendNeighbors(std::queue<std::pair<int, int>>& queue,
  const std::pair<int, int>& coords,
  int width,
  int height)
{
  for (int i = -1; i <= 1; i++)
  {
    for (int j = -1; j <= 1; j++)
    {
      if (i != 0 || j != 0)
      {
        const int nx = coords.first + i;
        const int ny = coords.second + j;
        if (0 <= nx && nx < width && 0 <= ny && ny < height)
        {
          queue.push(std::make_pair(nx, ny));
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
cluster_map clusterizeBinaryImage(vtkDataArray* values,
  int width,
  int height,
  unsigned long minPixelNumber)
{
  cluster_map clusters;

  vtkSmartPointer<vtkIntArray> labels = vtkSmartPointer<vtkIntArray>::New();
  labels->SetNumberOfValues(width * height);
  auto labelsRange = vtk::DataArrayValueRange(labels);
  vtkSMPTools::Fill(labelsRange.begin(), labelsRange.end(), -1);

  std::queue<std::pair<int, int>> queue;
  int label = 1;

  // For each pixel in the foreground, we will enque all of its non-processed neighbors and assign
  // them the same label This is the One component at a time algorithm explained here:
  // https://en.wikipedia.org/wiki/Connected-component_labeling#One_component_at_a_time
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      const int currentIndex = y * width + x;
      const int currentLabel = labels->GetValue(currentIndex);

      // If current is not processed
      if (currentLabel == -1)
      {
        const double currentValue = values->GetTuple1(currentIndex);
        // if current is foreground
        if (currentValue > 0)
        {
          clusters[label] = std::vector<std::pair<int, int>>();
          queue.push(std::make_pair(x, y));

          while (!queue.empty())
          {
            std::pair<int, int> elem = queue.front();
            queue.pop();

            const int eIndex = elem.second * width + elem.first;
            // If element is not processed
            if (labels->GetValue(eIndex) == -1)
            {
              // If element is foreground
              if (values->GetTuple1(eIndex) > 0)
              {
                labels->SetValue(eIndex, label);
                clusters[label].push_back(elem);

                appendNeighbors(queue, elem, width, height);
              }
              else
              {
                labels->SetValue(eIndex, 0);
              }
            }
          }
          label++;
        }
        else
        {
          labels->SetValue(currentIndex, 0);
        }
      }
    }
  }

  cluster_map filteredClusters;

  int k = 1;
  for (auto cluster : clusters)
  {
    if (cluster.second.size() > minPixelNumber)
    {
      filteredClusters[k++] = std::move(cluster.second);
    }
  }

  return filteredClusters;
}

//------------------------------------------------------------------------------
int vtkImageBasedClustering::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get inputs
  vtkPolyData* targetInput = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData* refInput = vtkPolyData::GetData(inputVector[1]->GetInformationObject(0));

  // Get outputs
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);
  vtkImageData* imageOutput = vtkImageData::GetData(outputVector, 1);
  vtkPolyData* cameraModelOutput = vtkPolyData::GetData(outputVector, 2);

  const int width = this->Resolution[0];
  const int height = this->Resolution[1];
  const int numberOfPixels = width * height;

  vtkSmartPointer<vtkPointCloudPinHoleProjector> projector0 =
    vtkSmartPointer<vtkPointCloudPinHoleProjector>::New();
  vtkSmartPointer<vtkPointCloudPinHoleProjector> projector1 =
    vtkSmartPointer<vtkPointCloudPinHoleProjector>::New();

  // Project reference point cloud
  projector0->SetResolution(this->Resolution);
  projector0->SetFocalLength(this->FocalLength);
  projector0->SetFillWithNeighbors(this->FillWithNeighbors);
  projector0->SetCameraTranslation(
    this->CameraTranslation[0], this->CameraTranslation[1], this->CameraTranslation[2]);
  projector0->SetCameraDirection(
    this->CameraDirection[0], this->CameraDirection[1], this->CameraDirection[2]);
  projector0->SetCameraAngle(this->CameraAngle);
  projector0->SetUseNaN(true);
  projector0->SetInputData(refInput);
  projector0->Update();

  // Project new point cloud
  projector1->SetResolution(this->Resolution);
  projector1->SetFocalLength(this->FocalLength);
  projector1->SetFillWithNeighbors(this->FillWithNeighbors);
  projector1->SetCameraTranslation(
    this->CameraTranslation[0], this->CameraTranslation[1], this->CameraTranslation[2]);
  projector1->SetCameraDirection(
    this->CameraDirection[0], this->CameraDirection[1], this->CameraDirection[2]);
  projector1->SetCameraAngle(this->CameraAngle);
  projector1->SetUseNaN(true);
  projector1->SetInputData(targetInput);
  projector1->Update();

  // Compute the difference between both
  vtkSmartPointer<vtkImagesOperations> imagesSubtractor =
    vtkSmartPointer<vtkImagesOperations>::New();
  imagesSubtractor->SetOperation(vtkImagesOperations::DIFF);
  vtkDataArray* arrayToProcess = this->GetInputArrayToProcess(0, inputVector);
  imagesSubtractor->SetInputArrayToProcess(
    arrayToProcess->GetName(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
  imagesSubtractor->SetInputConnection(0, projector0->GetOutputPort(0));
  imagesSubtractor->SetInputConnection(1, projector1->GetOutputPort(0));
  imagesSubtractor->SetResultScalarName("Difference");
  imagesSubtractor->Update();

  vtkImageData* img = imagesSubtractor->GetOutput();
  vtkPointData* pointData = img->GetPointData();
  pointData->SetActiveScalars("Difference");
  vtkDataArray* differenceArray = pointData->GetArray("Difference");

  // Init the difference array
  for (int i = 0; i < numberOfPixels; i++)
  {
    double val = differenceArray->GetTuple1(i);
    if (val != val)
    {
      differenceArray->SetTuple1(i, 0);
    }
  }

  // Binarize the image (0: background, 1: foreground)
  vtkSmartPointer<vtkImageThreshold> imageThreshold = vtkSmartPointer<vtkImageThreshold>::New();
  imageThreshold->SetInputData(img);
  imageThreshold->SetOutputScalarTypeToDouble();
  imageThreshold->ThresholdByLower(10.0);
  imageThreshold->ReplaceInOn();
  imageThreshold->SetInValue(0.0);
  imageThreshold->SetOutValue(255.0);
  imageThreshold->Update();

  // Erode the image to get "safer" centroids
  vtkSmartPointer<vtkImageDilateErode3D> erode = vtkSmartPointer<vtkImageDilateErode3D>::New();
  erode->SetInputConnection(imageThreshold->GetOutputPort());
  erode->SetDilateValue(0);
  erode->SetErodeValue(255);
  erode->SetKernelSize(3, 3, 1);
  erode->Update();

  vtkPointData* erodePd = erode->GetOutput()->GetPointData();
  vtkDoubleArray* binErode = vtkDoubleArray::SafeDownCast(erodePd->GetAbstractArray("Difference"));

  vtkPointData* imgThrPd = imageThreshold->GetOutput()->GetPointData();
  vtkDoubleArray* binImgThr =
    vtkDoubleArray::SafeDownCast(imgThrPd->GetAbstractArray("Difference"));

  // Compute clusters
  cluster_map clustersErrosion =
    clusterizeBinaryImage(binErode, width, height, this->MinNumberOfPixels);
  cluster_map clustersImgThr =
    clusterizeBinaryImage(binImgThr, width, height, this->MinNumberOfPixels);

  vtkSmartPointer<vtkPolyData> outPolyData = vtkSmartPointer<vtkPolyData>::New();
  outPolyData->ShallowCopy(targetInput);
  vtkPointData* outPointData = outPolyData->GetPointData();

  vtkPointData* image1Pd = projector1->GetOutput()->GetPointData();
  vtkIntArray* pointIdArray = vtkIntArray::SafeDownCast(image1Pd->GetAbstractArray("PointId"));

  // Put all points clustered using erosion in a PointData to use it in a vtkLocator
  vtkSmartPointer<vtkIntArray> erosionClusterIdArray = vtkSmartPointer<vtkIntArray>::New();
  vtkSmartPointer<vtkPointSet> locatorPointSet = vtkSmartPointer<vtkPointSet>::New();
  vtkSmartPointer<vtkPoints> locatorPoints = vtkSmartPointer<vtkPoints>::New();
  locatorPointSet->SetPoints(locatorPoints);
  for (auto cluster : clustersErrosion)
  {
    std::vector<std::pair<int, int>>& vect = cluster.second;
    for (size_t i = 0; i < vect.size(); i++)
    {
      std::pair<int, int> pixel = vect.at(i);
      const int x = pixel.first;
      const int y = pixel.second;

      const int imageIndex = y * width + x;
      const int pointId = pointIdArray->GetValue(imageIndex);

      if (pointId >= 0)
      {
        locatorPoints->InsertNextPoint(outPolyData->GetPoint(pointId));
        erosionClusterIdArray->InsertNextValue(cluster.first);
      }
    }
  }

  vtkSmartPointer<vtkIntArray> clustersArray = vtkSmartPointer<vtkIntArray>::New();
  int numberOfOutPoints = outPolyData->GetNumberOfPoints();
  clustersArray->SetName(this->ResultScalar.c_str());
  clustersArray->SetNumberOfValues(numberOfOutPoints);
  auto clusterRange = vtk::DataArrayValueRange(clustersArray);
  vtkSMPTools::Fill(clusterRange.begin(), clusterRange.end(), 0);

  if (clustersErrosion.size() > 0)
  {
    // Create & init the Locator
    vtkSmartPointer<vtkStaticPointLocator> locator = vtkSmartPointer<vtkStaticPointLocator>::New();
    locator->SetDataSet(locatorPointSet);
    locator->SetNumberOfPointsPerBucket(2);
    locator->BuildLocator();

    // Now, for each pixel in the non-eroded image:
    // Cluster it if its distance to an eroded point is lower than x
    double maxDistToErosionCluster = this->MaxDistToErosionCluster;
    for (auto cluster : clustersImgThr)
    {
      std::vector<std::pair<int, int>>& vect = cluster.second;
      for (size_t i = 0; i < vect.size(); i++)
      {
        std::pair<int, int> pixel = vect.at(i);
        const int x = pixel.first;
        const int y = pixel.second;

        const int imageIndex = y * width + x;
        const int pointId = pointIdArray->GetValue(imageIndex);

        if (pointId > 0)
        {
          const double* dilatedPoint = outPolyData->GetPoint(pointId);
          const int pid = locator->FindClosestPoint(dilatedPoint);

          if (pid >= 0)
          {
            const double* erodedPoint = locatorPointSet->GetPoint(pid);
            const int erodedLabel = erosionClusterIdArray->GetValue(pid);

            const double distance = std::sqrt(std::pow(dilatedPoint[0] - erodedPoint[0], 2) +
              std::pow(dilatedPoint[1] - erodedPoint[1], 2) +
              std::pow(dilatedPoint[2] - erodedPoint[2], 2));

            if (distance < maxDistToErosionCluster)
            {
              clustersArray->SetValue(pointId, erodedLabel);
            }
          }
        }
      }
    }
  }

  outPointData->AddArray(clustersArray);

  output->ShallowCopy(outPolyData);
  imageOutput->ShallowCopy(erode->GetOutput(0));
  cameraModelOutput->ShallowCopy(projector0->GetOutputDataObject(1));

  return VTK_OK;
}

//------------------------------------------------------------------------------
int vtkImageBasedClustering::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(1);
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