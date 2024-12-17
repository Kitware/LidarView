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
void vtkClusteringAndTracking::Gaussian3D::UpdateParams(const Eigen::Vector3d& pt, double weightX)
{
  // Update the weight
  double oldWeight = this->Weight;
  double sumWeight = this->NbInliers * oldWeight;
  this->Weight = (sumWeight + weightX) / (this->NbInliers + 1);

  // Update the mean
  Eigen::Vector3d oldMean = this->Mean;
  this->Mean = oldMean + weightX * (pt - oldMean) / (sumWeight + weightX);

  // Update the covariance using Welford's method
  // Do not update covariance for the clustering and tracking
  // to keep the cluster size

  // Update the number of sample
  ++NbInliers;
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::GaussianMixture3D::Reset()
{
  this->Gaussians.clear();
  this->UniqueID = 0;
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::GaussianMixture3D::GetClusters(
  std::vector<std::vector<int>>& clusters,
  std::vector<int>& clusterId)
{
  for (auto& gaussian : this->Gaussians)
  {
    if (gaussian.PointsId.empty())
      continue;
    clusters.emplace_back(gaussian.PointsId);
    clusterId.emplace_back(gaussian.Id);
  }
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::GaussianMixture3D::ClearClusters()
{
  for (auto& gaussian : this->Gaussians)
  {
    gaussian.PointsId.clear();
  }
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::GaussianMixture3D::AddPoint(const Eigen::Vector3d& pt, int ptId)
{
  // Create a new gaussian if the gaussian mixture model is empty
  if (this->Gaussians.empty())
  {
    Eigen::Matrix3d covInit = this->CovCoeff * Eigen::Matrix3d::Identity();
    Gaussian3D newGaussian(pt, covInit, this->MaxTTL, this->GetNewUniqueID());
    newGaussian.PointsId.emplace_back(ptId);
    this->Gaussians.push_back(newGaussian);
    this->ItClosest = this->Gaussians.begin();
    return;
  }
  // Find the closest gaussian for the new point
  std::vector<double> probas;
  double minDistance = FLT_MAX;
  this->ItClosest = this->Gaussians.begin();
  for (auto it = this->Gaussians.begin(); it != this->Gaussians.end(); ++it)
  {
    probas.emplace_back(it->ComputeWeightedProba(pt));
    double dist = (pt - it->Mean).norm();
    if (dist < minDistance)
    {
      minDistance = dist;
      this->ItClosest = it;
    }
  }

  // Update current gaussian mixture model if the new point is close to a gaussian cluster
  Eigen::Vector3d distance = pt - this->ItClosest->Mean;
  if (distance.norm() < 3. * this->GaussianClusterRadius)
  {
    this->ItClosest->PointsId.emplace_back(ptId);
    int idProba = 0;
    double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);
    for (auto& gaussian : this->Gaussians)
    {
      gaussian.UpdateParams(pt, probas[idProba] / sumProba);
      ++idProba;
    }
    this->ResetTTL();
    return;
  }

  // Create a new gaussian if the new point is far from existing distributions
  Eigen::Matrix3d covInit = this->CovCoeff * Eigen::Matrix3d::Identity();
  Gaussian3D newGaussian(
    pt, covInit, this->MaxTTL, this->GetNewUniqueID(), this->ItClosest->NbInliers + 1);
  newGaussian.PointsId.emplace_back(ptId);
  probas.clear();
  for (auto gaussian : this->Gaussians)
  {
    probas.emplace_back(gaussian(pt));
  }
  probas.emplace_back(newGaussian(pt));
  double sumProba = std::accumulate(probas.begin(), probas.end(), 0.);
  int idProba = 0;
  // Update existing gaussians with new weights
  for (auto& gaussian : this->Gaussians)
  {
    gaussian.UpdateParams(pt, probas[idProba] / sumProba);
    ++idProba;
  }
  // Set weight for new gaussian and add it to the model
  double newGaussianWeight = (probas.back() / sumProba) / (this->ItClosest->NbInliers + 1);
  newGaussian.Weight = newGaussianWeight;
  this->Gaussians.push_back(newGaussian);
  this->ItClosest = std::prev(this->Gaussians.end());
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::GaussianMixture3D::UpdateClusters()
{
  if (this->Gaussians.empty())
    return;
  auto it = this->Gaussians.begin();
  while (it != this->Gaussians.end())
  {
    // Check if or not the gaussian is still alive
    bool stillAlive = it->UpdateTTL();

    // Erase the gaussian if it is not alive
    if (!stillAlive || int(it->PointsId.size()) < this->GaussianClusterMinNbPoints)
    {
      // erase current iterator and get next on
      it = this->Gaussians.erase(it);
      // no need to increment iterator here
    }
    else
    {
      ++it; // increment iterator here
    }
  }
  // Normalise weights and clear points id
  int nbGaussians = this->Gaussians.size();
  for (auto& gaussian : this->Gaussians)
  {
    gaussian.NbInliers = 1;
    gaussian.Weight = 1. / double(nbGaussians);
  }
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::SetClusterExtractor(int extractor)
{
  Extractor clusterExtractor = static_cast<Extractor>(extractor);
  if (clusterExtractor != Extractor::NOEXTRACTION && clusterExtractor != Extractor::EUCLIDEAN &&
    clusterExtractor != Extractor::GMM && clusterExtractor != Extractor::REGION_GROWING)
  {
    vtkWarningMacro(<< "Invalid clustering extractor (" << extractor << "), ignoring setting.");
    return;
  }
  if (this->ClusterExtractor != extractor)
  {
    this->ClusterExtractor = clusterExtractor;
  }
  this->Modified();
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
void vtkClusteringAndTracking::ExtractClustersWithEuclidean(vtkSmartPointer<vtkPolyData> polydata,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput)
{
  if (polydata->GetNumberOfPoints() == 0)
  {
    vtkLog(INFO, "Not enough points: clusters not extracted");
    return;
  }

  // Extract cluster
  vtkNew<vtkEuclideanClusterExtraction> cluster;
  cluster->SetInputData(polydata);
  cluster->SetExtractionModeToAllClusters();
  cluster->SetRadius(this->ClusterRadius);
  cluster->ColorClustersOn();
  cluster->Update();

  // Create output with vertices
  polydata->ShallowCopy(cluster->GetOutput());
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(polydata->GetNumberOfPoints());
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(1, connectivity);
  polydata->SetVerts(cellArray);
  for (vtkIdType k = 0; k < polydata->GetNumberOfPoints(); ++k)
  {
    connectivity->SetValue(k, k);
  }

  // Get point indices of each cluster
  int numClusters = cluster->GetNumberOfExtractedClusters();
  std::unordered_map<int, std::vector<int>> clusters;
  clusters.reserve(numClusters);
  for (int clusterId = 0; clusterId < numClusters; ++clusterId)
  {
    for (vtkIdType pointId = 0; pointId < polydata->GetNumberOfPoints(); ++pointId)
    {
      auto currClusterId = polydata->GetPointData()->GetArray("ClusterId")->GetTuple1(pointId);
      clusters[currClusterId].emplace_back(pointId);
    }
  }

  // Compute cluster stats: size, mean depth, mean intensity etc
  this->ClustersStats.clear();
  for (const auto& clus : clusters)
  {
    auto& clusterId = clus.first;  // shortcut for clusterId
    auto& ptIndices = clus.second; // shortcut for point indices
    int nbClusterPoints = ptIndices.size();
    if (nbClusterPoints < this->ClusterMinNbPoints)
    {
      for (const auto& pointId : ptIndices)
      {
        polydata->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, -1);
      }
      continue;
    }

    this->ClustersStats.emplace_back(this->ComputeClusterStats(polydata, ptIndices, clusterId));
  }

  // Sort clusters based on their average depth values
  std::sort(this->ClustersStats.begin(),
    this->ClustersStats.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.MeanDepth < cluster2.MeanDepth; });
  std::vector<int> newClusterIds(numClusters);
  int newClusterId = 0;
  for (auto& cluster : this->ClustersStats)
  {
    newClusterIds[cluster.ClusterId] = newClusterId;
    ++newClusterId;
  }

  // Reset cluster id for points and cluster stats
  newClusterId = 0;
  for (auto& cluster : this->ClustersStats)
  {
    cluster.ClusterId = newClusterId;
    ++newClusterId;
  }
  auto clusterIdArray = polydata->GetPointData()->GetArray("ClusterId");
  for (vtkIdType pointId = 0; pointId < polydata->GetNumberOfPoints(); pointId++)
  {
    auto clusterId = clusterIdArray->GetTuple1(pointId);
    if (clusterId < 0)
      continue;
    polydata->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, newClusterIds[clusterId]);
  }

  // Create output of clusters information
  this->CreateClustersOutput(clustersOutput, infoOutput);
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

  // Extract clusters
  clustersPointsOutput->ShallowCopy(input);
  switch (this->ClusterExtractor)
  {
    case Extractor::EUCLIDEAN:
    {
      this->ExtractClustersWithEuclidean(clustersPointsOutput, clustersOutput, clusterInfoOutput);
      break;
    }
    case Extractor::GMM:
    {
      break;
    }
    case Extractor::REGION_GROWING:
    {
      break;
    }
    default:
      vtkWarningMacro(<< "No extractor selected");
  }
  return 1;
}
