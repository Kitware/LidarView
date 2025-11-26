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
#include "vtkLVUsedDependencies.h"
#include "vtkPointsPCA.h"

#ifdef LIDARVIEW_USE_NANOFLANN
#include "KDTreeVTKAdaptor.h"
#endif

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
#include <vtkVectorText.h>

// STD
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>

constexpr unsigned int POINTCLOUD_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int CLUSTERS_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int CLUSTERS_OUTPUT_PORT = 1;
constexpr unsigned int CLUSTERS_TEXT_OUTPUT_PORT = 2;
constexpr unsigned int CLUSTERS_3DTEXT_OUTPUT_PORT = 3;
constexpr unsigned int OUTPUT_PORT_COUNT = 4;

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
  if (port == CLUSTERS_3DTEXT_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
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
    if (!stillAlive ||
      static_cast<unsigned int>(it->PointsId.size()) < this->GaussianClusterMinNbPoints)
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
    clusterExtractor != Extractor::GMM && clusterExtractor != Extractor::REGION_GROWING &&
    clusterExtractor != Extractor::ADAPTIVE_EUCLIDEAN)
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

//----------------------------------------------------------------------------
void vtkClusteringAndTracking::SetClusterRadius(double radius)
{
  if (this->ClusterRadius != radius)
  {
    this->ClusterRadius = radius;
    if (this->ClusterExtractor == Extractor::GMM)
    {
      this->GMMClusters.SetGaussianClusterRadius(radius);
      this->GMMClusters.SetCovCoeff(radius);
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkClusteringAndTracking::SetClusterMinNbPoints(unsigned int minNbPoints)
{
  if (this->ClusterMinNbPoints != minNbPoints)
  {
    this->ClusterMinNbPoints = minNbPoints;
    if (this->ClusterExtractor == Extractor::GMM)
    {
      this->GMMClusters.SetGaussianClusterMinNbPoints(minNbPoints);
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkClusteringAndTracking::SetTrackingWindowSizes(int trackingWindowSizes)
{
  if (this->TrackingWindowSizes != trackingWindowSizes)
  {
    this->TrackingWindowSizes = trackingWindowSizes;
    if (this->ClusterExtractor == Extractor::GMM)
    {
      this->GMMClusters.SetMaxTTL(trackingWindowSizes);
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkClusteringAndTracking::SetClusterGridResolution(float resolution)
{
  if (resolution <= 0)
  {
    vtkWarningMacro(<< "Clustering grid resolution can not be 0 or negative!");
    return;
  }
  if (this->ClusterGridResolution != resolution)
  {
    this->ClusterGridResolution = resolution;
    this->ClustersGrid.VoxelMap.clear();
    for (int i = 0; i < 3; i++)
    {
      this->ClustersGrid.GridSize[i] = this->ClustersGrid.GridSize[i] *
        this->ClustersGrid.Resolution / this->ClusterGridResolution;
    }
    this->ClustersGrid.Resolution = this->ClusterGridResolution;
  }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::SetClusterInfoToDisplay(int displayInfo)
{
  DisplayStat clusterInfo = static_cast<DisplayStat>(displayInfo);
  if (this->ClusterInfoToDisplay != clusterInfo)
  {
    this->ClusterInfoToDisplay = clusterInfo;
  }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::InitClusteringGrid(vtkPolyData* polydata)
{
  // Initialize parameters of clustering grid
  double bounds[6];
  polydata->GetBounds(bounds);
  this->ClustersGrid.Origin = { bounds[0], bounds[2], bounds[4] };
  this->ClustersGrid.Resolution = this->ClusterGridResolution;
  for (int i = 0; i < 3; i++)
  {
    this->ClustersGrid.GridSize[i] =
      std::ceil((bounds[2 * i + 1] - bounds[2 * i]) / this->ClustersGrid.Resolution);
  }
  this->ClustersGrid.IsInitialized = true;
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::BuildBackgroundGrid(vtkPolyData* polydata)
{
  // Update background grid
  Eigen::Vector3d point;
  for (vtkIdType id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    // Count number of points for each voxel
    polydata->GetPoint(id, point.data());
    Eigen::Array3i coords =
      ((point - this->ClustersGrid.Origin) / this->ClustersGrid.Resolution).cast<int>();
    if (this->ClustersGrid.IsInBounds(coords))
      ++this->ClustersGrid.BackgroudMap[this->ClustersGrid.To1d(coords)].NbCurrentPts;
  }
  for (auto& el : this->ClustersGrid.BackgroudMap)
  {
    auto& vox = el.second;
    if (vox.NbCurrentPts > 0)
      ++vox.SeenTimes;
    vox.NbCurrentPts = 0;
  }

  // Remove voxels which are not considered as part of the background
  // at the end of initialization phase
  if (this->NbProcessedFrames == this->InitNbFrames)
  {
    // Remove voxels which are not considered as part of the background
    // at the end of initialization phase
    for (auto it = this->ClustersGrid.BackgroudMap.begin();
         it != this->ClustersGrid.BackgroudMap.end();)
    {
      Voxel& vox = it->second; // shortcut
      // A voxel is not background if it has been seen less than 5% of initial time
      if (vox.SeenTimes < 0.05 * this->InitNbFrames)
      {
        it = this->ClustersGrid.BackgroudMap.erase(it);
        continue;
      }
      ++it;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::Reset()
{
  this->NbProcessedFrames = 0;
  if (this->ClusterExtractor == Extractor::GMM)
  {
    this->GMMClusters.Reset();
  }
  if (this->ClusterExtractor == Extractor::REGION_GROWING)
  {
    this->ClustersGrid.VoxelMap.clear();
    this->ClustersGrid.BackgroudMap.clear();
    this->ClustersGrid.IsInitialized = false;
    this->NewClusterIdx = 0;
  }
}

//-----------------------------------------------------------------------------
double vtkClusteringAndTracking::ComputeDepth(vtkSmartPointer<vtkPolyData> input, const int pointId)
{
  double depth = 0.;
  if (this->DepthArrayName.empty())
  {
    Eigen::Vector3d point;
    input->GetPoint(pointId, point.data());
    depth = point.norm();
  }
  else
  {
    depth = input->GetPointData()->GetArray(this->DepthArrayName.c_str())->GetTuple1(pointId);
  }
  return depth;
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
    Eigen::Vector3d point;
    input->GetPoint(pointId, point.data());
    clusterPoints->InsertNextPoint(point.data());
    depth += this->ComputeDepth(input, pointId);
    if (!this->IntensityArrayName.empty())
      intensity +=
        input->GetPointData()->GetArray(this->IntensityArrayName.c_str())->GetTuple1(pointId);
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
  // Get height value
  clusterInfo.Height = clusterInfo.BoundingBox.GetSize().z();
  return clusterInfo;
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::RedistributeClusterIdByDepth(std::vector<int>& newClusterIds)
{
  // Sort clusters based on their average depth values
  std::sort(this->ClustersStats.begin(),
    this->ClustersStats.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.MeanDepth < cluster2.MeanDepth; });
  // Save new cluster id
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
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::CreateClustersOutput(
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput,
  vtkSmartPointer<vtkMultiBlockDataSet> clusters3DTextOutput)
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

    // Create 3d text to display cluster information
    vtkNew<vtkVectorText> text3D;
    std::ostringstream txt;
    txt << this->ClusterDisplayStatNames.at(this->ClusterInfoToDisplay) << ": "
        << std::setprecision(2);
    switch (this->ClusterInfoToDisplay)
    {
      case CLUSTER_ID:
        txt << cluster.ClusterId;
        break;
      case NB_POINTS:
        txt << cluster.NbPoints;
        break;
      case HEIGHT:
        txt << cluster.Height << " m";
        break;
      case DEPTH:
        txt << cluster.MeanDepth << " m";
        break;
      case INTENSITY:
        txt << cluster.MeanIntensity;
        break;
    }
    text3D->SetText(txt.str().c_str());
    text3D->Update();

    vtkNew<vtkTransform> transform;
    auto center = cluster.BoundingBox.GetTrueCenter();
    transform->Translate(center.x(), center.y(), center.z() + cluster.Height * 0.6);
    transform->RotateX(90);
    transform->RotateY(-90);
    transform->Scale(0.5, 0.5, 0.5);

    vtkNew<vtkTransformPolyDataFilter> tfText;
    tfText->SetInputConnection(text3D->GetOutputPort());

    tfText->SetTransform(transform);
    tfText->Update();

    vtkSmartPointer<vtkPolyData> labelPoly = tfText->GetOutput();
    clusters3DTextOutput->SetBlock(blockId, labelPoly);
    std::string labelblockName("Cluster-" + std::to_string(cluster.ClusterId));
    clusters3DTextOutput->GetMetaData(blockId)->Set(
      vtkCompositeDataSet::NAME(), labelblockName.c_str());
    clusters3DTextOutput->GetBlock(blockId)->SetFieldData(fieldData);
    blockId++;
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
  vtkSmartPointer<vtkTable> infoOutput,
  vtkSmartPointer<vtkMultiBlockDataSet> clusters3DTextOutput)
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
    if (ptIndices.size() < this->ClusterMinNbPoints)
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
  std::vector<int> newClusterIds(numClusters);
  this->RedistributeClusterIdByDepth(newClusterIds);
  // Label points with new clusterId
  auto clusterIdArray = polydata->GetPointData()->GetArray("ClusterId");
  for (vtkIdType pointId = 0; pointId < polydata->GetNumberOfPoints(); pointId++)
  {
    auto clusterId = clusterIdArray->GetTuple1(pointId);
    if (clusterId < 0)
      continue;
    polydata->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, newClusterIds[clusterId]);
  }

  // Create output of clusters information
  this->CreateClustersOutput(clustersOutput, infoOutput, clusters3DTextOutput);
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::ExtractClustersWithGMM(vtkSmartPointer<vtkPolyData> polydata,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput,
  vtkSmartPointer<vtkMultiBlockDataSet> clusters3DTextOutput)
{
  if (polydata->GetNumberOfPoints() == 0)
  {
    vtkLog(INFO, "Not enough points: clusters not extracted");
    return;
  }

  // Extract clusters with gaussian mixture model
  Eigen::Vector3d point;
  for (int id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    polydata->GetPoint(id, point.data());
    this->GMMClusters.AddPoint(point, id);
  }
  this->GMMClusters.UpdateClusters();

  // Add array to store cluster Id
  // The array name is ClusterId to be same as the vtkEuclideanClusterExtraction filter
  vtkSmartPointer<vtkIntArray> pointLabel = vtkSmartPointer<vtkIntArray>::New();
  pointLabel->SetName("ClusterId");
  for (auto id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    pointLabel->InsertNextTuple1(-1);
  }
  polydata->GetPointData()->AddArray(pointLabel);

  // Compute cluster stats: size, mean depth, mean intensity etc
  std::vector<std::vector<int>> clusters;
  std::vector<int> clusterUUID;
  this->GMMClusters.GetClusters(clusters, clusterUUID);
  this->ClustersStats.clear();
  for (unsigned int id = 0; id < clusters.size(); ++id)
  {
    if (clusters[id].size() < this->ClusterMinNbPoints)
      continue;

    // Label points with their cluster Id
    for (const auto& pointId : clusters[id])
    {
      polydata->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, clusterUUID[id]);
    }

    // Compute stats
    this->ClustersStats.emplace_back(
      this->ComputeClusterStats(polydata, clusters[id], clusterUUID[id]));
  }
  this->GMMClusters.ClearClusters();

  // Sort clusters based on their clusterId
  std::sort(this->ClustersStats.begin(),
    this->ClustersStats.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.ClusterId < cluster2.ClusterId; });

  // Create output of clusters information
  this->CreateClustersOutput(clustersOutput, infoOutput, clusters3DTextOutput);
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::ExtractClustersWithRegionGrowing(
  vtkSmartPointer<vtkPolyData> polydata,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput,
  vtkSmartPointer<vtkMultiBlockDataSet> clusters3DTextOutput)
{
  if (polydata->GetNumberOfPoints() == 0)
  {
    vtkLog(INFO, "Not enough points: clusters not extracted");
    return;
  }

  // Initialize clusters grid
  if (!this->ClustersGrid.IsInitialized)
    this->InitClusteringGrid(polydata);

  // Add array to store cluster Id
  // The array name is ClusterId to be same as the vtkEuclideanClusterExtraction filter
  vtkSmartPointer<vtkIntArray> pointLabel = vtkSmartPointer<vtkIntArray>::New();
  pointLabel->SetName("ClusterId");
  for (auto id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    pointLabel->InsertNextTuple1(-1);
  }
  polydata->GetPointData()->AddArray(pointLabel);

  // Add point into voxel grid to extract cluster
  Eigen::Vector3d point;
  for (int id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    polydata->GetPoint(id, point.data());
    // Compute 3D coordinates
    Eigen::Array3i coords =
      ((point - this->ClustersGrid.Origin) / this->ClustersGrid.Resolution).cast<int>();
    if (!this->ClustersGrid.IsInBounds(coords))
      continue;
    // Store point indices of each voxel
    this->ClustersGrid(coords).CurrentPtIndices.emplace_back(id);
  }

  int currentFrame = this->NbProcessedFrames;
  int unknownLabel = -1;
  // Label time for voxels
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    Voxel& vox = el.second; // shortcut
    if (!vox.CurrentPtIndices.empty())
    {
      vox.ClusterIdx = unknownLabel;
      vox.Time = currentFrame;
    }
  }

  // Link cluster idx to grid indices to speed up search
  std::unordered_map<int, std::vector<int>> clus2gridIndices;
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    int voxelIdx = el.first;  // shortcut
    Voxel& voxel = el.second; // shortcut
    if (voxel.ClusterIdx < 0)
      continue;
    clus2gridIndices[voxel.ClusterIdx].push_back(voxelIdx);
  }
  // Compute centroids of clusters voxel
  std::unordered_map<int, Eigen::Vector3d> clusCentroid;
  clusCentroid.reserve(clus2gridIndices.size());
  for (auto& el : clus2gridIndices)
  {
    auto& clusIdx = el.first;  // shortcut
    auto& indices = el.second; // shortcut
    Eigen::Vector3d centroid(0., 0., 0.);
    for (auto& idx : indices)
    {
      centroid += this->ClustersGrid.To3d(idx).cast<double>().matrix();
    }
    centroid *= this->ClustersGrid.Resolution;
    centroid /= clus2gridIndices[clusIdx].size();
    clusCentroid[clusIdx] = centroid;
  }

  // clang-format off
  const std::vector<Eigen::Array3i> radii = {{ 1,  1, 1}, { 1,  1, 0}, { 1,  1, -1},
                                             { 1,  0, 1}, { 1,  0, 0}, { 1,  0, -1},
                                             { 1, -1, 1}, { 1, -1, 0}, { 1, -1, -1},
                                             { 0,  1, 1}, { 0,  1, 0}, { 0,  1, -1},
                                             { 0,  0, 1},              { 0,  0, -1},
                                             { 0, -1, 1}, { 0, -1, 0}, { 0, -1, -1},
                                             {-1,  1, 1}, {-1,  1, 0}, {-1,  1, -1},
                                             {-1,  0, 1}, {-1,  0, 0}, {-1,  0, -1},
                                             {-1, -1, 1}, {-1, -1, 0}, {-1, -1, -1}};
  // clang-format on
  // Grow existing regions
  for (auto& el : clus2gridIndices)
  {
    auto& clusIdx = el.first;  // shortcut
    auto& indices = el.second; // shortcut

    std::vector<Eigen::Array3i> clusterVoxels;
    clusterVoxels.reserve(indices.size());
    // Fill region with existing pixels
    for (int idx : indices)
      clusterVoxels.push_back(this->ClustersGrid.To3d(idx));

    // Grow region
    int idxVox = 0;
    while (idxVox < int(clusterVoxels.size()))
    {
      // Add neighbors
      for (const Eigen::Array3i& r : radii)
      {
        Eigen::Array3i neigh = clusterVoxels[idxVox] + r;
        if (!this->ClustersGrid.IsInBounds(neigh))
          continue;
        // Check neighbor validity
        if (!this->ClustersGrid.Check(neigh))
          continue;

        // If neighbor is occupied, add it to current cluster
        int neighClusIdx = this->ClustersGrid(neigh).ClusterIdx;

        if (neighClusIdx == unknownLabel)
        {
          // Add neighbor to cluster
          clusterVoxels.push_back(neigh);
          // Update label
          this->ClustersGrid(neigh).ClusterIdx = clusIdx;
          continue;
        }

        // If the neighbor belongs to another existing cluster
        // Check the distance between their centroids
        if (neighClusIdx != clusIdx)
        {
          auto& neighCentroid = clusCentroid[neighClusIdx];
          auto& currCentroid = clusCentroid[clusIdx];
          // Merge clusters if they are close
          if ((neighCentroid - currCentroid).norm() < this->ClusterRadius)
          {
            // Merge the new cluster to the current one
            for (int idx : clus2gridIndices[neighClusIdx])
            {
              // Add to cluster
              clusterVoxels.emplace_back(this->ClustersGrid.To3d(idx));
              // Update label
              this->ClustersGrid.VoxelMap[idx].ClusterIdx = clusIdx;
            }
            // Remove neighbor cluster from clus2gridIndices
            // to not process it afterwards
            clus2gridIndices.erase(neighClusIdx);
          }
        }
      }
      ++idxVox;
    }
  }

  // Fill the potential seeds with remaining unknown pixels
  std::vector<Eigen::Array3i> potentialSeeds;
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    int idx = el.first;     // shortcut
    Voxel& vox = el.second; // shortcut
    if (vox.ClusterIdx == unknownLabel)
      potentialSeeds.push_back(this->ClustersGrid.To3d(idx));
  }
  // Grow new regions
  for (const auto& seed : potentialSeeds)
  {
    // Check seed is still unknown
    if (this->ClustersGrid(seed).ClusterIdx >= 0)
      continue;

    this->ClustersGrid(seed).ClusterIdx = this->NewClusterIdx;

    std::vector<Eigen::Array3i> clusterVoxels;
    clusterVoxels.reserve(this->ClustersGrid.VoxelMap.size());
    clusterVoxels.emplace_back(seed);
    int idxVox = 0;
    while (idxVox < int(clusterVoxels.size()))
    {
      // Check neighbors
      for (const Eigen::Array3i& r : radii)
      {
        Eigen::Array3i neigh = clusterVoxels[idxVox] + r;
        if (!this->ClustersGrid.IsInBounds(neigh))
          continue;
        // If neighbor is occupied, add it to current cluster
        if (this->ClustersGrid.Check(neigh) && this->ClustersGrid(neigh).ClusterIdx == unknownLabel)
        {
          // Add neighbor to cluster
          clusterVoxels.push_back(neigh);
          // Update cluster label of pixel
          this->ClustersGrid(neigh).ClusterIdx = this->NewClusterIdx;
        }
      }
      ++idxVox;
    }
    ++this->NewClusterIdx;
  }

  // Check the validity of clusters with background map
  std::unordered_map<int, bool> clusValidity;
  if (this->EnableBackgroundGrid)
  {
    std::unordered_map<int, int> nbPtsInBackgroundMap;
    std::unordered_map<int, int> nbPtsInCluster;
    for (auto& el : this->ClustersGrid.VoxelMap)
    {
      auto& mapId = el.first;
      Voxel& vox = el.second; // shortcut
      if (vox.ClusterIdx < 0)
        continue;
      if (!nbPtsInBackgroundMap.count(vox.ClusterIdx))
        nbPtsInBackgroundMap[vox.ClusterIdx] = 0;
      if (this->ClustersGrid.BackgroudMap.count(mapId))
      {
        ++nbPtsInBackgroundMap[vox.ClusterIdx];
      }
      ++nbPtsInCluster[vox.ClusterIdx];
    }
    for (auto& [clusIdx, back] : nbPtsInBackgroundMap)
    {
      clusValidity[clusIdx] = (float(back) / float(nbPtsInCluster[clusIdx]) > 0.5) ? false : true;
    }
  }

  // Remove old and non-valid voxels
  for (auto it = this->ClustersGrid.VoxelMap.begin(); it != this->ClustersGrid.VoxelMap.end();)
  {
    Voxel& vox = it->second; // shortcut
    if (currentFrame - vox.Time > this->TrackingWindowSizes ||
      (this->EnableBackgroundGrid && !clusValidity[vox.ClusterIdx]))
    {
      // Erase and return the iterator to the next element
      it = this->ClustersGrid.VoxelMap.erase(it);
      continue;
    }
    // Only increment if not erasing
    ++it;
  }

  // Store point indices for each cluster
  std::unordered_map<int, std::vector<int>> clus2Points;
  for (auto& el : this->ClustersGrid.VoxelMap)
  {
    Voxel& voxel = el.second; // shortcut
    if (voxel.ClusterIdx < 0)
    {
      voxel.CurrentPtIndices.clear();
      continue;
    }
    for (const auto& ptId : voxel.CurrentPtIndices)
    {
      clus2Points[voxel.ClusterIdx].emplace_back(ptId);
    }
    // Clear current points of each pixel for next fill
    voxel.CurrentPtIndices.clear();
  }

  // Compute cluster stats: size, mean depth, mean intensity etc
  this->ClustersStats.clear();
  for (const auto& clusPts : clus2Points)
  {
    auto& clusId = clusPts.first;
    auto& clusPtsId = clusPts.second;

    if (clusPtsId.size() < this->ClusterMinNbPoints)
      continue;

    // Label points with their cluster Id
    for (const auto& pointId : clusPtsId)
    {
      polydata->GetPointData()->GetArray("ClusterId")->SetTuple1(pointId, clusId);
    }

    // Compute stats
    this->ClustersStats.emplace_back(this->ComputeClusterStats(polydata, clusPtsId, clusId));
  }

  // Sort clusters based on their clusterId
  std::sort(this->ClustersStats.begin(),
    this->ClustersStats.end(),
    [](const ClusterStats& cluster1, const ClusterStats& cluster2)
    { return cluster1.ClusterId < cluster2.ClusterId; });

  // Create output of clusters information
  this->CreateClustersOutput(clustersOutput, infoOutput, clusters3DTextOutput);
}

//-----------------------------------------------------------------------------
void vtkClusteringAndTracking::ExtractClustersWithAdaptiveEuclidean(
  vtkSmartPointer<vtkPolyData> polydata,
  vtkSmartPointer<vtkMultiBlockDataSet> clustersOutput,
  vtkSmartPointer<vtkTable> infoOutput,
  vtkSmartPointer<vtkMultiBlockDataSet> clusters3DTextOutput)
{
  // Set input data
  if (polydata->GetNumberOfPoints() == 0)
  {
    vtkLog(INFO, "Not enough points: clusters not extracted");
    return;
  }
  // Add array to store cluster Id
  // The array name is ClusterId to be same as the vtkEuclideanClusterExtraction filter
  vtkSmartPointer<vtkIntArray> pointLabel = vtkSmartPointer<vtkIntArray>::New();
  pointLabel->SetName("ClusterId");
  for (auto id = 0; id < polydata->GetNumberOfPoints(); ++id)
  {
    pointLabel->InsertNextTuple1(-1);
  }
  polydata->GetPointData()->AddArray(pointLabel);

#ifdef LIDARVIEW_USE_NANOFLANN
  vtkKDTreeVTKAdaptor kDTree;
  kDTree.Reset(polydata);
  auto numPoints = polydata->GetNumberOfPoints();
  std::vector<bool> visited(numPoints, false);
  std::vector<std::vector<int>> clusters;
  // Clustering with an adaptive search radius depending on depth
  for (auto id = 0; id < numPoints; ++id)
  {
    if (visited[id])
      continue;
    // Get depth value
    double depth = this->ComputeDepth(polydata, id);
    // Compute the search radius depending on depth
    float radius = this->ClusterRadius + this->Factor * depth;
    // Clustering
    std::vector<int> cluster;
    std::queue<int> candidatePts;
    candidatePts.push(id);
    visited[id] = true;
    while (!candidatePts.empty())
    {
      int idx = candidatePts.front();
      candidatePts.pop();

      Eigen::Vector3d point;
      polydata->GetPoint(idx, point.data());
      std::vector<float> sqDistances;
      std::vector<int> neighborsIndices;
      Eigen::Vector3f pointF = point.cast<float>();
      kDTree.radiusSearch(pointF.data(), radius, neighborsIndices, sqDistances);
      for (const int neighborId : neighborsIndices)
      {
        if (!visited[neighborId])
        {
          visited[neighborId] = true;
          candidatePts.push(neighborId);
          cluster.push_back(neighborId);
        }
      }
    }
    if (cluster.size() > static_cast<size_t>(this->ClusterMinNbPoints))
      clusters.emplace_back(cluster);
  }

  // Compute cluster stats: size, mean depth, mean intensity etc
  this->ClustersStats.clear();
  int clusterId = 0;
  for (const auto& clus : clusters)
  {
    this->ClustersStats.emplace_back(this->ComputeClusterStats(polydata, clus, clusterId));
    clusterId++;
  }

  // Sort clusters based on their average depth values
  std::vector<int> newClusterIds(clusters.size());
  this->RedistributeClusterIdByDepth(newClusterIds);
  // Label points with new clusterId
  for (size_t i = 0; i < clusters.size(); ++i)
  {
    int clusterId = newClusterIds[i];
    for (const auto pId : clusters[i])
    {
      polydata->GetPointData()->GetArray("ClusterId")->SetTuple1(pId, clusterId);
    }
  }
#else
  vtkErrorMacro(
    << "Cannot extract clusters with adaptive euclidean method because nanoflann was not found.");
#endif

  // Create output of clusters information
  this->CreateClustersOutput(clustersOutput, infoOutput, clusters3DTextOutput);
}

//-----------------------------------------------------------------------------
int vtkClusteringAndTracking::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkLog(INFO, "Clustering and tracking for frame #" << this->NbProcessedFrames);

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
  vtkMultiBlockDataSet* clusters3DTextOutput =
    vtkMultiBlockDataSet::GetData(outputVector, CLUSTERS_3DTEXT_OUTPUT_PORT);

  // Get array name
  this->DepthArrayName =
    this->GetInputArrayToProcess(0, input) ? this->GetInputArrayToProcess(0, input)->GetName() : "";
  this->IntensityArrayName =
    this->GetInputArrayToProcess(1, input) ? this->GetInputArrayToProcess(1, input)->GetName() : "";

  // Extract clusters
  clustersPointsOutput->ShallowCopy(input);
  switch (this->ClusterExtractor)
  {
    case Extractor::EUCLIDEAN:
    {
      this->ExtractClustersWithEuclidean(
        clustersPointsOutput, clustersOutput, clusterInfoOutput, clusters3DTextOutput);
      break;
    }
    case Extractor::GMM:
    {
      this->ExtractClustersWithGMM(
        clustersPointsOutput, clustersOutput, clusterInfoOutput, clusters3DTextOutput);
      break;
    }
    case Extractor::REGION_GROWING:
    {
      if (this->EnableBackgroundGrid && this->NbProcessedFrames <= this->InitNbFrames)
        this->BuildBackgroundGrid(clustersPointsOutput);
      this->ExtractClustersWithRegionGrowing(
        clustersPointsOutput, clustersOutput, clusterInfoOutput, clusters3DTextOutput);
      break;
    }
    case Extractor::ADAPTIVE_EUCLIDEAN:
    {
      this->ExtractClustersWithAdaptiveEuclidean(
        clustersPointsOutput, clustersOutput, clusterInfoOutput, clusters3DTextOutput);
      break;
    }
    default:
      vtkWarningMacro(<< "No extractor selected");
  }

  ++this->NbProcessedFrames;

  return 1;
}
