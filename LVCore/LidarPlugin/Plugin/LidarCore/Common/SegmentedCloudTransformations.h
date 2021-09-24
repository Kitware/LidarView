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



#ifndef SEGMENTED_CLOUD_TRANSFORMATIONS_H
#define SEGMENTED_CLOUD_TRANSFORMATIONS_H

// YAML
#include <yaml-cpp/yaml.h>

//LOCAL
#include "CategoriesConfig.h"
#include "Common/BoundingBox.h"
#include "LidarCoreModule.h"

// VTK
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

#include <Eigen/Dense>

// STD
#include <map>


// Templates

/**
 * @brief Median value of a std::vector
 */
template<typename T>
T Median(const std::vector<T> in_vect)
{
  std::vector<T> out_vect = in_vect;
  sort(out_vect.begin(), out_vect.end());

  if (out_vect.size() % 2 == 0)
  {
    return (out_vect[out_vect.size() / 2] + out_vect[out_vect.size() / 2 + 1])/ 2;
  }
  else
  {
    return out_vect[out_vect.size() / 2] ;
  }
}

/**
  * @brief mostCommon returns most common element in a iterable object
  * Adapted from https://stackoverflow.com/questions/2488941/find-which-numbers-appears-most-in-a-vector
  */
 template<class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
T mostCommon(InputIt begin, InputIt end)
{
  std::map<T, int> counts;
  for (InputIt it = begin; it != end; ++it) {
    if (counts.find(*it) != counts.end()) {
      ++counts[*it];
    }
    else {
      counts[*it] = 1;
    }
  }
  return std::max_element(counts.begin(), counts.end(),
    [] (const std::pair<T, int>& pair1, const std::pair<T, int>& pair2) {
    return pair1.second < pair2.second;})->first;
}


/**
 * @class Segment
 * @brief Segment from point cloud: This class contains all the attributes
 *        of a point cloud segment:
 *        - attributes from the categories config file (ignore, isThing, segmentId, ...)
 *        - list of point belonging to this segment
 */
class Segment
{
public:
  // Attributes from CategoriesConfig
  bool ignore;
  bool isThing;
  int segmentId;
  int categoryId;
  float confidence;

  // Attributes from point cloud
  std::vector<unsigned int> pointIndices;

  Segment() : ignore(true), isThing(false), segmentId(-1), categoryId(0), confidence(-1.0) {}

  Segment(YAML::Node segment, CategoriesConfig* catConfig)
  {
    int categoryId = segment["category_id"].as<int>();
    int segmentId = segment["id"].as<int>();
    this->ignore = catConfig->IsCategoryIgnored(categoryId);
    this->isThing = catConfig->IsCategoryThing(categoryId);
    this->segmentId = segmentId;
    this->categoryId = categoryId;
    if (this->isThing)
    {
    this->confidence = segment["score"].as<float>();
    }
  }
};

/**
 * @class SegmentedCloud
 * @brief SegmentedCloud provides an API to apply transformations to point clouds
 *        using their segmentation values.
 *        Example usage:
*         """
 *        SegmentedCloud segmentedCloud(cloud, yamlSegments, catConfig);
 *        segmentedCloud.transformation1();
 *        segmentedCloud.transformation2();
 *        ...
 *        segmentedCloud.UpdateCloud();
 *        segmentedCloud.GetPointCloud();
 *        """
*/
class LIDARCORE_EXPORT SegmentedCloud
{
public:

  SegmentedCloud() { };
  ~SegmentedCloud() { };

  SegmentedCloud(vtkSmartPointer<vtkPolyData> cloud, std::vector<Segment> segments);
  SegmentedCloud(vtkSmartPointer<vtkPolyData> cloud,
                 YAML::Node yamlSegments,
                 CategoriesConfig* catConfig);


  /**
   * @brief ApplyKNNsToSegments Smooth segments using k-nearest neighbors
   *
   * @param nbNeighbors number of neighbors (k)
   * @param maxDistance maximum distance from neighbors for them to be considered
   */
  void ApplyKNNsToSegments(size_t nbNeighbors,
                           double maxDistance=std::numeric_limits<double>::max());

  /**
   * @brief ApplyDistanceThreshold Remove points closer than threshold from the origin
   *
   * @param threshold minimum distance from the origin (in meters) to keep points in
   * segments
   */
  void ApplyDistanceThreshold(float threshold=0);

  /**
   * @brief RemoveSegmentsOutliers Remove outliers from each segment using Z-score
   *        See https://medium.com/james-blogs/outliers-make-us-go-mad-univariate-outlier-detection-b3a72f1ea8c7
   */
  void RemoveSegmentsOutliers();

  /**
   * @brief RemoveSegmentsWithLessPointsThanThreshold Remove from the list of segments all those
   * that contain less than 'threshold' points
   * @param threshold minimum number of point per segment
   */
  void RemoveSegmentsWithLessPointsThanThreshold(int threshold=5);

  /**
   * @brief CreateBoundingBoxes Create 3D OrientedBoundingBoxes for each
   *        segment that has isThing=1
   *        As the computation of bounding boxes around point clouds is axis
   *        aligned, a rotation matrix can be provided to compute the bounding
   *        boxes in a rotated reference frame
   * @param BboxesRotation Rotation matrix for the bounding boxes reference frame
   *        (default is identity)
   */
  std::vector<OrientedBoundingBox<3>> CreateBoundingBoxes(Eigen::Matrix3d BboxesRotation=Eigen::Matrix3d::Identity());

  /**
   * @brief UpdateCloud Update point cloud segment values according to the Segments values
   */
  void UpdateCloud();

  vtkSmartPointer<vtkPolyData> GetPointCloud()
    {return this->PointCloud; };

protected:
  vtkSmartPointer<vtkPolyData> PointCloud = vtkSmartPointer<vtkPolyData>::New();
  std::vector<Segment> Segments;
};

#endif // SEGMENTED_CLOUD_TRANSFORMATIONS_H

