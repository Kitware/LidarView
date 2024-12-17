/*=========================================================================

  Program: LidarView
  Module:  vtkClusteringAndTracking.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClusteringAndTracking.h"
#include "vtkPointsPCA.h"

// VTK
#include <vtkAppendFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCenterOfMass.h>
#include <vtkCleanPolyData.h>
#include <vtkCubeSource.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkEuclideanClusterExtraction.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPCAStatistics.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>
#include <vtkQuaternion.h>
#include <vtkRadiusOutlierRemoval.h>
#include <vtkRemovePolyData.h>
#include <vtkStringArray.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>

// STD
#include <iomanip>
#include <iostream>
#include <list>
#include <numeric>

constexpr unsigned int POINTCLOUD_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int CLUSTERS_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int CLUSTERS_OUTPUT_PORT = 1;
constexpr unsigned int CLUSTERS_TEXT_OUTPUT_PORT = 2;
constexpr unsigned int OUTPUT_PORT_COUNT = 3;

// Implementation of the New function
vtkStandardNewMacro(vtkClusteringAndTracking)

//----------------------------------------------------------------------------
vtkClusteringAndTracking::vtkClusteringAndTracking()
{
  // One input port
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);

  // The accumulation of stabilized frames
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//----------------------------------------------------------------------------
vtkClusteringAndTracking::~vtkClusteringAndTracking() = default;

//-----------------------------------------------------------------------------
int vtkClusteringAndTracking::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkClusteringAndTracking::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == CLUSTERS_POINTS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == CLUSTERS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
    return 1;
  }
  if (port == CLUSTERS_TEXT_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
Eigen::Vector3d vtkClusteringAndTracking::Bbox::GetTrueCenter() const
{
  return this->GetEigenTransform() * this->Center;
}

//-----------------------------------------------------------------------------
Eigen::Vector4d vtkClusteringAndTracking::Bbox::GetOrientation() const
{
  Eigen::Isometry3d transform = this->GetEigenTransform();
  Eigen::Quaterniond quat(transform.linear());
  Eigen::Vector4d out;
  out.tail(4) << quat.x(), quat.y(), quat.z(), quat.w();
  out.tail(4).normalize();
  return out;
}

//-----------------------------------------------------------------------------
Eigen::Isometry3d vtkClusteringAndTracking::Bbox::GetEigenTransform() const
{
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.linear().col(0) =
    Eigen::Vector3d(this->Transform[0], this->Transform[4], this->Transform[8]);
  transform.linear().col(1) =
    Eigen::Vector3d(this->Transform[1], this->Transform[5], this->Transform[9]);
  transform.linear().col(2) =
    Eigen::Vector3d(this->Transform[2], this->Transform[6], this->Transform[10]);
  transform.translation() =
    Eigen::Vector3d(this->Transform[3], this->Transform[7], this->Transform[11]);
  return transform;
}

//-----------------------------------------------------------------------------
vtkClusteringAndTracking::ClusterStats vtkClusteringAndTracking::ComputeClusterStats(
  vtkSmartPointer<vtkPolyData> input,
  const std::vector<int>& clusterPtIndices,
  const int clusterId)
{
  // Create polydata for points of clusters
  vtkNew<vtkPoints> clusterPoints;
  vtkNew<vtkPolyData> clusterPointsPolydata;
  clusterPointsPolydata->SetPoints(clusterPoints);

  ClusterStats clusterInfo;
  // Calculate the average depth value and bounding box for this cluster
  double depth = 0.0;
  double intensity = 0.0;
  for (const auto& pointId : clusterPtIndices)
  {
    depth += input->GetPointData()->GetArray("distance_m")->GetTuple1(pointId);
    intensity += input->GetPointData()->GetArray("intensity")->GetTuple1(pointId);
    Eigen::Vector3d point;
    input->GetPoint(pointId, point.data());
    clusterPoints->InsertNextPoint(point.data());
  }
  int nbClusterPoints = clusterPtIndices.size();
  clusterInfo.ClusterId = clusterId;
  clusterInfo.NbPoints = nbClusterPoints;
  depth /= nbClusterPoints;
  intensity /= nbClusterPoints;
  clusterInfo.MeanDepth = depth;
  clusterInfo.MeanIntensity = intensity;

  if (this->EnableClusterOrientation)
  {
    // For PCA
    auto computePCA = vtkSmartPointer<vtkPointsPCA>::New();
    computePCA->SetInputData(clusterPointsPolydata);
    computePCA->Update();
    vtkNew<vtkDoubleArray> eigenvectors;
    computePCA->GetEigenVectors(eigenvectors);

    Eigen::Vector3d pointcloudCentroid;
    vtkCenterOfMass::ComputeCenterOfMass(clusterPoints, nullptr, pointcloudCentroid.data());

    // clang-format off
    double matrix[16] = {
      eigenvectors->GetValue(6), eigenvectors->GetValue(3), eigenvectors->GetValue(0), pointcloudCentroid.x(),
      eigenvectors->GetValue(7), eigenvectors->GetValue(4), eigenvectors->GetValue(1), pointcloudCentroid.y(),
      eigenvectors->GetValue(8), eigenvectors->GetValue(5), eigenvectors->GetValue(2), pointcloudCentroid.z(),
      0., 0., 0., 1. };
    // clang-format on
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(matrix);

    // Transform cluster point cloud
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    transformFilter->SetTransform(transform->GetInverse());
    transformFilter->SetInputData(clusterPointsPolydata);
    transformFilter->Update();
    double bounds[6];
    transformFilter->GetOutput()->GetBounds(bounds);

    clusterInfo.BoundingBox.SetTransform(matrix);
    clusterInfo.BoundingBox.SetVertices(
      bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
  }
  else
  {
    double bounds[6];
    clusterPointsPolydata->GetBounds(bounds);
    clusterInfo.BoundingBox.SetVertices(
      bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
  }

  // Label cluster by geometry dimension
  // clang-format off
  if (clusterInfo.BoundingBox.GetSize().z() >= 0.8 && clusterInfo.BoundingBox.GetSize().z() <= 2.2 &&
     ((clusterInfo.BoundingBox.GetSize().x() >= 0.2 && clusterInfo.BoundingBox.GetSize().x() <= 0.7) ||
      (clusterInfo.BoundingBox.GetSize().y() >= 0.2 && clusterInfo.BoundingBox.GetSize().y() <= 0.7)))
  // clang-format on
  {
    clusterInfo.ClusterLabel = vtkClusteringAndTracking::Label::HUMAN;
  }
  else
  {
    clusterInfo.ClusterLabel = vtkClusteringAndTracking::Label::OTHERS;
  }
  return clusterInfo;
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::CreateClustersOutput(
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput)
{
  // Add bounding box for each cluster into output
  int blockId = 0;
  for (const auto& cluster : this->ClustersStats)
  {
    vtkNew<vtkCubeSource> cubeSource;
    vtkNew<vtkPolyData> source;
    cubeSource->SetBounds(cluster.BoundingBox.GetVertices().data());
    cubeSource->SetCenter(cluster.BoundingBox.GetCenter().data());
    if (this->EnableClusterOrientation)
    {
      // Transform bounding box
      vtkNew<vtkTransformPolyDataFilter> transformFilter;
      vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
      transform->SetMatrix(cluster.BoundingBox.GetTransform().data());
      transformFilter->SetTransform(transform);
      transformFilter->SetInputConnection(cubeSource->GetOutputPort());
      transformFilter->Update();
      source->ShallowCopy(transformFilter->GetOutput());
    }
    else
    {
      cubeSource->Update();
      source->ShallowCopy(cubeSource->GetOutput());
    }

    std::string blockName("Cluster-" + std::to_string(cluster.ClusterId));
    clustersOutput->SetBlock(blockId, source);
    clustersOutput->GetMetaData(blockId)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());

    // Create field data and add information to it
    vtkSmartPointer<vtkIntArray> bboxId = vtkSmartPointer<vtkIntArray>::New();
    bboxId->SetName("ClusterId");
    bboxId->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxDistances = vtkSmartPointer<vtkFloatArray>::New();
    bboxDistances->SetName("Distance");
    bboxDistances->SetNumberOfComponents(1);
    vtkSmartPointer<vtkFloatArray> bboxSizes = vtkSmartPointer<vtkFloatArray>::New();
    bboxSizes->SetName("Size");
    bboxSizes->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxCenters = vtkSmartPointer<vtkFloatArray>::New();
    bboxCenters->SetName("Center");
    bboxCenters->SetNumberOfComponents(3);
    vtkSmartPointer<vtkFloatArray> bboxOrientations = vtkSmartPointer<vtkFloatArray>::New();
    bboxOrientations->SetName("Orientation");
    bboxOrientations->SetNumberOfComponents(4);
    vtkSmartPointer<vtkUnsignedShortArray> bboxLabels =
      vtkSmartPointer<vtkUnsignedShortArray>::New();
    bboxLabels->SetName("Label");
    bboxLabels->SetNumberOfComponents(1);

    vtkSmartPointer<vtkFieldData> fieldData = vtkSmartPointer<vtkFieldData>::New();
    fieldData->AddArray(bboxId);
    fieldData->AddArray(bboxDistances);
    fieldData->AddArray(bboxSizes);
    fieldData->AddArray(bboxCenters);
    fieldData->AddArray(bboxOrientations);
    fieldData->AddArray(bboxLabels);
    bboxId->InsertNextTuple1(static_cast<int>(cluster.ClusterId));
    bboxDistances->InsertNextTuple1(cluster.MeanDepth);
    bboxSizes->InsertNextTuple(cluster.BoundingBox.GetSize().data());
    bboxCenters->InsertNextTuple(cluster.BoundingBox.GetTrueCenter().data());
    bboxOrientations->InsertNextTuple(cluster.BoundingBox.GetOrientation().data());
    bboxLabels->InsertNextTuple1(static_cast<unsigned short>(cluster.ClusterLabel));

    clustersOutput->GetBlock(blockId)->SetFieldData(fieldData);

    ++blockId;
  }

  // Print clusters info
  // Set output for displaying clusters information
  vtkNew<vtkStringArray> data;
  data->SetName("Clusters Information");
  data->SetNumberOfComponents(1);
  std::ostringstream oss;
  for (const auto& cluster : this->ClustersStats)
  {
    oss << std::setprecision(3) << std::showpoint << "Cluster " << std::setw(2) << cluster.ClusterId
        << ": distance = " << std::setw(7) << cluster.MeanDepth << "m  "
        << "size = " << cluster.BoundingBox.GetSize().transpose() << "\n";
  }
  std::string clusterInfo = oss.str();
  data->InsertNextValue(clusterInfo.c_str());
  infoOutput->AddColumn(data);
}

//-----------------------------------------------------------------------------
int vtkClusteringAndTracking::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get input data
  vtkPolyData* input =
    vtkPolyData::GetData(inputVector[POINTCLOUD_INPUT_PORT]->GetInformationObject(0));
  // Check if input is a multiblock
  if (!input)
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::GetData(inputVector[POINTCLOUD_INPUT_PORT], 0);
    // Extract first block if it is a vtkPolyData
    input = vtkPolyData::SafeDownCast(mb->GetBlock(0));
  }
  // If the input could not be cast, return
  if (!input || input->GetNumberOfPoints() < 1)
  {
    vtkErrorMacro(<< "Unable to cast input into a vtkPolyData");
    return 0;
  }

  // Get the output
  vtkPolyData* clustersPointsOutput =
    vtkPolyData::GetData(outputVector, CLUSTERS_POINTS_OUTPUT_PORT);
  vtkMultiBlockDataSet* clustersOutput =
    vtkMultiBlockDataSet::GetData(outputVector, CLUSTERS_OUTPUT_PORT);
  vtkTable* clusterInfoOutput = vtkTable::GetData(outputVector, CLUSTERS_TEXT_OUTPUT_PORT);

  return 1;
}
