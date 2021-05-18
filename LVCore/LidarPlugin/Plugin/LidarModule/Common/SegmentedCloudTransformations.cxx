//=========================================================================
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

// YAML
#include <yaml-cpp/yaml.h>

//LOCAL
#include "CategoriesConfig.h"
#include "Common/BoundingBox.h"
#include "SegmentedCloudTransformations.h"
#include "vtkBoundingBox.h"
#include "KDTreeVectorOfVectorsAdaptor.h"
#include "vtkHelper.h"

// STD
#include <math.h>

// VTK
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>


// Helper functions
namespace {

void RemovePointsCloserThanThreshold(Segment* segment,
                                     vtkSmartPointer<vtkPolyData> cloud,
                                     float threshold=0)
{
  auto distanceArray = cloud->GetPointData()->GetArray("distance_m");
  std::vector<unsigned int> remainPointIndices(0);
  for (size_t i = 0; i < segment->pointIndices.size(); ++i)
  {
    if (distanceArray->GetTuple1(segment->pointIndices[i]) >= threshold)
    {
      remainPointIndices.push_back(segment->pointIndices[i]);
    }
  }
  segment->pointIndices = remainPointIndices;
}


std::vector<bool> GetOutliersInCloudSubset(vtkSmartPointer<vtkPolyData> cloud,
                                           std::vector<unsigned int> subsetPointIndices,
                                           double cutoff=3.5)
{
  size_t nbPoints = subsetPointIndices.size();
  if (nbPoints == 0)
  {
    return {};
  }

  // Compute position of median point
  std::vector<double> Xs(nbPoints);
  std::vector<double> Ys(nbPoints);
  std::vector<double> Zs(nbPoints);

  // TODO : check if median of independant coordinates matches median point
  // maybe https://en.wikipedia.org/wiki/Geometric_median is better
  for (size_t i = 0; i < nbPoints; ++i)
  {
    double pt[3];
    cloud->GetPoint(subsetPointIndices[i], pt);
    Xs[i] = pt[0];
    Ys[i] = pt[1];
    Zs[i] = pt[2];
  }
  Eigen::Vector3d medPt(Median(Xs), Median(Ys), Median(Zs));

  // Compute Median distance to medPt
  std::vector<double> distToMedPt(nbPoints);
  for (size_t i = 0; i < nbPoints; ++i)
  {
    double pt[3];
    cloud->GetPoint(subsetPointIndices[i], pt);
    Eigen::Vector3d X(pt[0] - medPt[0], pt[1] - medPt[1], pt[2] - medPt[2]);
    distToMedPt[i] = X.norm();
  }
  // TODO: check case mad = 0
  double mad = Median(distToMedPt);

  // Modified Z-score
  std::vector<double> modifiedZScore(nbPoints);
  for (size_t i = 0; i < nbPoints; ++i)
  {
    double pt[3];
    cloud->GetPoint(subsetPointIndices[i], pt);
    // 0.6745 is the 0.75th quartile of the standard normal distribution, to which the MAD converges to.
    modifiedZScore[i] = (0.6745 * distToMedPt[i]) / mad;
  }

  // Determine outliers
  std::vector<bool> isOutLier(nbPoints, true);
  for (size_t i = 0; i < nbPoints; ++i)
  {
    if (modifiedZScore[i] <= cutoff)
    {
      isOutLier[i] = false;
    }
  }

  return isOutLier;
}


void RemoveOutliers(Segment* segment, vtkSmartPointer<vtkPolyData> cloud)
{
  std::vector<unsigned int> remainPointIndices(0);
  std::vector<bool> isOutlier = GetOutliersInCloudSubset(cloud, segment->pointIndices);
  for (size_t i = 0; i < segment->pointIndices.size(); ++i)
  {
    if (!isOutlier[i])
    {
      remainPointIndices.push_back(segment->pointIndices[i]);
    }
  }
  segment->pointIndices = remainPointIndices;
}


std::vector<Segment> GetSegmentsWithMorePtsThanThreshold(std::vector<Segment> segmentsList,
                                                         unsigned int threshold = 0)
{
  std::vector<unsigned int> remainSegmentIndices(0);
  for (size_t i = 0; i < segmentsList.size(); ++i)
  {
    if (segmentsList[i].pointIndices.size() >= threshold)
    {
      remainSegmentIndices.push_back(i);
    }
  }
  std::vector<Segment> outSegmentsList;
  for (auto i: remainSegmentIndices)
  {
    outSegmentsList.push_back(segmentsList[i]);
  }

  return outSegmentsList;
}


OrientedBoundingBox<3> CreateBboxFromSegment(Segment* segment,
                             vtkSmartPointer<vtkPolyData> cloud,
                             Eigen::Matrix3d BboxesRotation=Eigen::Matrix3d::Identity())
{
  vtkBoundingBox vtkBbox;
  vtkDataArray* timeArray = cloud->GetPointData()->GetArray("adjustedtime");
  // TODO: find a better time than the one of the first point
  int time = timeArray->GetTuple1(segment->pointIndices[0]);

  // vtkBbox aligned to interpolator axes (rotation only)
  for (size_t i = 0; i < segment->pointIndices.size(); ++i)
  {
    double pt[3];
    cloud->GetPoint(segment->pointIndices[i], pt);
    Eigen::Vector3d X(pt[0], pt[1], pt[2]);
    Eigen::Vector3d X1 = BboxesRotation.transpose() * X;
    vtkBbox.AddPoint(X1[0],X1[1], X1[2]);
  }

  double center[3];
  vtkBbox.GetCenter(center);
  Eigen::Vector3d C(center[0], center[1], center[2]);

  double bounds[6]; // double xMin, double xMax, double yMin, double yMax, double zMin, double zMax
  vtkBbox.GetBounds(bounds);

  Eigen::Vector3d D(bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]);

  // Create bbox in original coordinates
  OrientedBoundingBox<3> bbox;
  bbox.Center = BboxesRotation * C;
  bbox.Width =  D;
  bbox.Orientation = BboxesRotation;
  bbox.ClassId = segment->categoryId;
  bbox.SegmentId = segment->segmentId;
  bbox.Confidence = static_cast<int>(segment->confidence * 100);
  bbox.TimeStamp = time;

  return bbox;
}

} // end namespace


// Constructors
SegmentedCloud::SegmentedCloud(vtkSmartPointer<vtkPolyData> cloud, std::vector<Segment> segments)
{
  this->PointCloud->DeepCopy(cloud);
  this->Segments = segments;
}

SegmentedCloud::SegmentedCloud(vtkSmartPointer<vtkPolyData> cloud,
                               YAML::Node yamlSegments,
                               CategoriesConfig* catConfig)
{
  this->PointCloud->DeepCopy(cloud);

  // Initialise segments
  std::vector<Segment> segmentsList;
  for (size_t segmentIdx = 0; segmentIdx < yamlSegments.size(); ++segmentIdx)
  {
    this->Segments.push_back(Segment(yamlSegments[segmentIdx], catConfig));
  }

  // Populate segments
  vtkDataArray* segmentArray = cloud->GetPointData()->GetArray("segment");
  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    int ptSegmentId = segmentArray->GetTuple1(pointIdx);
    for (size_t i = 0; i < this->Segments.size(); ++i)
    {
      if (ptSegmentId == this->Segments[i].segmentId)
      {
        this->Segments[i].pointIndices.push_back(pointIdx);
      }
    }
  }
}


// -----------------------------------------------------------------------------

void SegmentedCloud::ApplyKNNsToSegments(size_t  nbNeighbors, double maxDistance)
{
  // Create Segment list with empty indices
  std::vector<Segment> outSegments = this->Segments;
  std::map<int, size_t> segmentIdMap;
  for (size_t i = 0; i < outSegments.size(); ++i)
  {
    outSegments[i].pointIndices = {};
    segmentIdMap.insert(std::make_pair(outSegments[i].segmentId, i));
  }

  // Prepare KD-tree
  std::vector<std::vector<double>> points;
  std::vector<int> idxArray;
  for (unsigned int pointIdx = 0; pointIdx < this->PointCloud->GetNumberOfPoints(); ++pointIdx)
  {
    double* p = this->PointCloud->GetPoint(pointIdx);
    std::vector<double> apoint = {p[0], p[1], p[2]};
    points.push_back(apoint);
    idxArray.push_back(pointIdx);
  }
  typedef std::vector<std::vector<double> > my_vector_of_vectors_t;
  typedef KDTreeVectorOfVectorsAdaptor< my_vector_of_vectors_t, double > my_kd_tree_t;

  my_kd_tree_t mat_index(-1 /*dim*/, points, 10 /* max leaf */ );
  mat_index.index->buildIndex();

  vtkDataArray* segmentArray = this->PointCloud->GetPointData()->GetArray("segment");

  // Use apply nearest neighbors segment value

  for (unsigned int pointIdx = 0; pointIdx < this->PointCloud->GetNumberOfPoints(); ++pointIdx)
  {
    // TODO: Add distance weights for nearest neighbors values
    std::vector<size_t> nearestIndex(nbNeighbors, -1);
    std::vector<double> nearestDist(nbNeighbors, -1.0);
    std::vector<int> nearestSegments(0);

    mat_index.query(points[pointIdx].data(), nbNeighbors, nearestIndex.data(), nearestDist.data());

    // get segment value for neighbors
    for (size_t i = 0; i < nbNeighbors; ++i)
    {
      if ((nearestDist[i] >= 0) && (nearestDist[i] <= maxDistance)) {
        nearestSegments.push_back(segmentArray->GetTuple1(nearestIndex[i]));
      }
    }
    if (nearestSegments.size() > 0)
    {
      int mostCommonSegment = mostCommon(nearestSegments.begin(), nearestSegments.end());

      // Add point to corresponding segment
      if (segmentIdMap.find(mostCommonSegment) != segmentIdMap.end())
      {
        outSegments[segmentIdMap[mostCommonSegment]].pointIndices.push_back(pointIdx);
      }
    }
  }

  this->Segments = outSegments;
}

void SegmentedCloud::ApplyDistanceThreshold(float threshold)
{
  for (size_t segmentIdx = 0; segmentIdx < this->Segments.size(); ++segmentIdx)
  {
    RemovePointsCloserThanThreshold(&(this->Segments[segmentIdx]), this->PointCloud, threshold);
  }

}

void SegmentedCloud::RemoveSegmentsOutliers()
{
  for (size_t segmentIdx = 0; segmentIdx < this->Segments.size(); ++segmentIdx)
  {
    RemoveOutliers(&(this->Segments[segmentIdx]), this->PointCloud);
  }
}

void SegmentedCloud::RemoveSegmentsWithLessPointsThanThreshold(int threshold)
{
  this->Segments = GetSegmentsWithMorePtsThanThreshold(this->Segments, threshold);


}

std::vector<OrientedBoundingBox<3>> SegmentedCloud::CreateBoundingBoxes(Eigen::Matrix3d BboxesRotation)
{
  // Create bboxes for "thing" segments only
  std::vector<OrientedBoundingBox<3>> objects;
  for (size_t segmentIdx = 0; segmentIdx < this->Segments.size(); ++segmentIdx)
  {
    if (this->Segments[segmentIdx].isThing
        && !this->Segments[segmentIdx].ignore
        && this->Segments[segmentIdx].pointIndices.size() > 0)
    {
      auto object = CreateBboxFromSegment(&(this->Segments[segmentIdx]), this->PointCloud, BboxesRotation);
      if (object.Width.norm() <= 20)  // remove too big bboxes
      {
        objects.push_back(object);
      }
    }
  }
  return objects;
}

void SegmentedCloud::UpdateCloud()
{
  this->PointCloud->GetPointData()->RemoveArray("segment");
  this->PointCloud->GetPointData()->RemoveArray("category");

  vtkSmartPointer<vtkIntArray> segmentArray = addArrayWithDefault<vtkIntArray, int>("segment", this->PointCloud, 0);
  vtkSmartPointer<vtkIntArray> categoryArray = addArrayWithDefault<vtkIntArray, int>("category", this->PointCloud, 0);

  for (size_t segmentIdx = 0; segmentIdx < this->Segments.size(); ++segmentIdx)
  {
    for (auto pointIdx : this->Segments[segmentIdx].pointIndices)
    {
      segmentArray->SetTuple1(pointIdx, this->Segments[segmentIdx].segmentId);
      categoryArray->SetTuple1(pointIdx, this->Segments[segmentIdx].categoryId);
    }
  }
}
