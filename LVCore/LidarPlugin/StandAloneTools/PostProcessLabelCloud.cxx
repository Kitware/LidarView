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

/* This Standalone tool takes segmented point clouds (e. g. outputs of LabelCloudFromImagesDetections)
and cleans them up using 3D geometry hints + create 3D bounding boxes for instance segments

It successively:
  - applies KNN to segment values in order to fill potential gaps in the annotation
  - removes points that are closer than a given threshold
  - remove outliers for each instance segment using z-score
  - Remove instance segments with number of points under a given threshold
  - saves 3D bounding boxes (not oriented for now)


   Usage:
   ./install/bin/PostProcessLabelCloud \
      <first frame to process>  \
      <last frame to process> \
      <lidar frames series> \
      <trajectory file> \
      <categories config file> \
      <cloud output folder> \
      <bboxes output folder> \
      <bboxes algo name> \
      <segment series file>
    ...

    with:
      - first frame to process: index of the first frame to process
      - last frame to process: index of the last frame to process (can be -1)
      - lidar frames series: full path to a frame.vtp.series file containing the names and
          times for the lidar extracted frames series
      - trajectory file: full path to a trajectory.vtp file.
          Used to orient bounding boxes aginst the trajectory
          If set to "", the bounding boxes are oriented against the default axes
      - categories config file: full path to a config file as described in CategoriesConfig.h
      - cloud output folder: full path to the folder to save the modified clouds to
      - bboxes output folder: full path to the folder to save the 3D bounding boxes to
      - bboxes algo name: algorithm name to set for the 3D bounding boxes
      - segment series file: full path to a segment.yml.serues file containing the pointes to the
          *.yml files with segments information for each cloud frame
          The structure of those files is like the following example:
                  file_name: frame_0000.vtp
                  image_id: '000001' # Id for the corresponding image if the source is 2D segmentation
                  segments_info:
                  - category_id: 3
                    id: 1
                    instance_id: 0
                    score: 0.9953383207321167
                  - category_id: 3
                    id: 2
                    instance_id: 1
 */


// LOCAL
#include "vtkPCLConversions.h"
#include "vtkTemporalTransforms.h"
#include "vtkCustomTransformInterpolator.h"
#include "CameraModel.h"
#include "vtkBoundingBoxReader.h"
#include "vtkBoundingBox.h"
#include "CameraProjection.h"
#include "BoundingBox.h"
#include "vtkEigenTools.h"
#include "CategoriesConfig.h"
#include "FramesSeriesUtils.h"
#include "KDTreeVectorOfVectorsAdaptor.h"

// STD
#include <iostream>
#include <math.h>

#include <cassert>

// BOOST
#include <boost/filesystem.hpp>

// YAML
#include <yaml-cpp/yaml.h>

// VTK
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkJPEGReader.h>
#include <vtkPNGReader.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkFieldData.h>
#include <vtk_jsoncpp.h>
#include <vtkHelper.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>

// Helper classes -------------------------------------------------------------
class Bbox3d
{
public:
  Eigen::Vector3d center;
  Eigen::Vector3d dimensions;
  Eigen::Vector3d rotation;
  int confidence;
  int class_id;
  int time;

  Bbox3d() : center(0, 0, 0), dimensions(2.0, 2.0, 2.0), rotation(0, 0, 0),
             confidence(0), time(0.) {};
};

class Segment
{
public:
  bool ignore;
  bool isThing;
  int segmentId;
  int categoryId;
  std::vector<unsigned int> pointIndices;
  float confidence;

  Segment() : ignore(true), isThing(false), segmentId(-1), categoryId(0), confidence(-1) {}

  Segment(YAML::Node segment, CategoriesConfig*  catConfig)
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

// Outliers filtering functions ----------------------------------------------

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

// Inspired from https://medium.com/james-blogs/outliers-make-us-go-mad-univariate-outlier-detection-b3a72f1ea8c7
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


void RemovePointsCloserThanThreshold(Segment* segment,
                                     vtkSmartPointer<vtkPolyData> cloud,
                                     float threshold = 0)
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


// Bbox utils -----------------------------------------------------------------

Bbox3d CreateBboxFromSegment(Segment* segment,
                             vtkSmartPointer<vtkPolyData> cloud,
                             vtkSmartPointer<vtkCustomTransformInterpolator> interpolator=NULL)
{


  vtkBoundingBox vtkBbox;
  vtkDataArray* timeArray = cloud->GetPointData()->GetArray("adjustedtime");
  // TODO: find a better time than the one of the first point
  int time = timeArray->GetTuple1(segment->pointIndices[0]);

  Eigen::Matrix3d R = Eigen::Matrix3d::Identity();

  if (interpolator)
  {
    std::pair<Eigen::Matrix3d, Eigen::Vector3d> H0 = GetRTFromTime(interpolator, 1e-6 * static_cast<double>(time));
    R = H0.first;
  }

  // Get Heading direction (in 2D)
  if (interpolator)
  {
    std::pair<Eigen::Matrix3d, Eigen::Vector3d> H0 = GetRTFromTime(interpolator, 1e-6 * static_cast<double>(time));
    std::pair<Eigen::Matrix3d, Eigen::Vector3d> H1 = GetRTFromTime(interpolator, 1e-6 * static_cast<double>(time) + 1);
    Eigen::Vector3d heading = H1.second - H0.second;
    double yaw = std::atan2(heading[1], heading[0]);
    double pitch = -std::asin(heading[2]/ heading.norm());
    double roll = 0;
    R = RollPitchYawToMatrix(roll, pitch, yaw);
  }

  // vtkBbox aligned to interpolator axes (rotation only)
  for (size_t i = 0; i < segment->pointIndices.size(); ++i)
  {
    double pt[3];
    cloud->GetPoint(segment->pointIndices[i], pt);
    Eigen::Vector3d X(pt[0], pt[1], pt[2]);
    Eigen::Vector3d X1 = R.transpose() * X;
    vtkBbox.AddPoint(X1[0],X1[1], X1[2]);
  }

  double center[3];
  vtkBbox.GetCenter(center);
  Eigen::Vector3d C(center[0], center[1], center[2]);

  double bounds[6]; // double xMin, double xMax, double yMin, double yMax, double zMin, double zMax
  vtkBbox.GetBounds(bounds);

  Eigen::Vector3d D(bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]);

  // Create bbox in original coordinates
  Bbox3d bbox;
  bbox.center = R * C;
  bbox.dimensions =  D;
  bbox.rotation = 180.0 / vtkMath::Pi() * MatrixToRollPitchYaw(R);
  bbox.class_id = segment->categoryId;
  bbox.confidence = static_cast<int>(segment->confidence * 100);
  bbox.time = time;

  return bbox;
}


void Export3DBBAsYaml(std::vector<Bbox3d> objects,
                      std::string outputYamlFile, CategoriesConfig* catConfig,
                      std::string algo_name)
{
  YAML::Node ymlFile;
  ymlFile["meta"] = YAML::Node();
  ymlFile["objects"] = YAML::Node();

  ymlFile["meta"]["date"] = std::string("N.A.");
  ymlFile["meta"]["source"] = std::string("N.A.");

  for (size_t i = 0; i < objects.size(); ++i)
  {
    YAML::Node currentBB;

    currentBB["label"] = catConfig->GetSuperCategory(objects[i].class_id);

    currentBB["custom"] = YAML::Node();
    currentBB["custom"]["confidence"] = objects[i].confidence;
    currentBB["custom"]["algo"] = algo_name;
    currentBB["custom"]["adjustedtime"] = objects[i].time;

    currentBB["selector"] = YAML::Node();
    currentBB["selector"]["center"].push_back(objects[i].center(0));
    currentBB["selector"]["center"].push_back(objects[i].center(1));
    currentBB["selector"]["center"].push_back(objects[i].center(2));

    currentBB["selector"]["dimensions"].push_back(objects[i].dimensions(0));
    currentBB["selector"]["dimensions"].push_back(objects[i].dimensions(1));
    currentBB["selector"]["dimensions"].push_back(objects[i].dimensions(2));

    currentBB["selector"]["rotation"].push_back(objects[i].rotation(0));
    currentBB["selector"]["rotation"].push_back(objects[i].rotation(1));
    currentBB["selector"]["rotation"].push_back(objects[i].rotation(2));

    currentBB["selector"]["type"] = "3D bounding box";

    ymlFile["objects"].push_back(currentBB);
  }

  std::ofstream fout(outputYamlFile.c_str());
  fout << ymlFile;
}

// -----------------------------------------------------------------------------

// https://stackoverflow.com/questions/2488941/find-which-numbers-appears-most-in-a-vector
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


std::vector<Segment> ApplyKNNsToSegments(vtkSmartPointer<vtkPolyData> cloud,
                                         std::vector<Segment> segmentsList,
                                         size_t nbNeighbors,
                                         double maxDistance=std::numeric_limits<double>::max()
                                         )
{
  // Create Segment list with empty indices
  std::vector<Segment> outSegmentsList = segmentsList;
  std::map<int, size_t> segmentIdMap;
  for (size_t i = 0; i < outSegmentsList.size(); ++i)
  {
    outSegmentsList[i].pointIndices = {};
    segmentIdMap.insert(std::make_pair(outSegmentsList[i].segmentId, i));
  }

  // Prepare KD-tree
  std::vector<std::vector<double>> points;
  std::vector<int> idxArray;
  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    double* p = cloud->GetPoint(pointIdx);
    std::vector<double> apoint = {p[0], p[1], p[2]};
    points.push_back(apoint);
    idxArray.push_back(pointIdx);
  }
  typedef std::vector<std::vector<double> > my_vector_of_vectors_t;
  typedef KDTreeVectorOfVectorsAdaptor< my_vector_of_vectors_t, double > my_kd_tree_t;

  my_kd_tree_t mat_index(-1 /*dim*/, points, 10 /* max leaf */ );
  mat_index.index->buildIndex();

  vtkDataArray* segmentArray = cloud->GetPointData()->GetArray("segment");

  // Use apply nearest neighbors segment value

  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
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
        outSegmentsList[segmentIdMap[mostCommonSegment]].pointIndices.push_back(pointIdx);
      }
    }
  }
  return outSegmentsList;
}

// -----------------------------------------------------------------------------


void OverwriteCloudSegmentInfos(std::vector<Segment> segmentsList,
                                vtkSmartPointer<vtkPolyData> outCloud)
{
  outCloud->GetPointData()->RemoveArray("segment");
  outCloud->GetPointData()->RemoveArray("category");

  vtkSmartPointer<vtkIntArray> segmentArray = addArrayWithDefault<vtkIntArray, int>("segment", outCloud, 0);
  vtkSmartPointer<vtkIntArray> categoryArray = addArrayWithDefault<vtkIntArray, int>("category", outCloud, 0);

  for (size_t segmentIdx = 0; segmentIdx < segmentsList.size(); ++segmentIdx)
  {
    for (auto pointIdx : segmentsList[segmentIdx].pointIndices)
    {
      segmentArray->SetTuple1(pointIdx, segmentsList[segmentIdx].segmentId);
      categoryArray->SetTuple1(pointIdx, segmentsList[segmentIdx].categoryId);
    }
  }
}

// ----------------------------------------------------------------------------

void RefineLabelCloudWithInstances(vtkSmartPointer<vtkPolyData> cloud,
                                   YAML::Node yamlSegments,
                                   vtkSmartPointer<vtkPolyData> outCloud,  // Will store refined Cloud
                                   std::vector<Bbox3d>* objects,   // Will store instances 3D Bboxes
                                   CategoriesConfig*  catConfig,
                                   vtkSmartPointer<vtkCustomTransformInterpolator> interpolator=NULL)
{
  // Initialise segments
  std::vector<Segment> segmentsList;
  for (size_t segmentIdx = 0; segmentIdx < yamlSegments.size(); ++segmentIdx)
  {
    segmentsList.push_back(Segment(yamlSegments[segmentIdx], catConfig));
  }

  // Populate segments
  vtkDataArray* segmentArray = cloud->GetPointData()->GetArray("segment");
  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    int ptSegmentId = segmentArray->GetTuple1(pointIdx);
    for (size_t i = 0; i < segmentsList.size(); ++i)
    {
      if (ptSegmentId == segmentsList[i].segmentId)
      {
        segmentsList[i].pointIndices.push_back(pointIdx);
      }
    }
  }

  // Refine segments
  int nbNeighbors = 10;
  double maxDistance = 0.3; // in meters
  segmentsList = ApplyKNNsToSegments(cloud, segmentsList, nbNeighbors, maxDistance);

  for (size_t segmentIdx = 0; segmentIdx < segmentsList.size(); ++segmentIdx)
  {
    RemovePointsCloserThanThreshold(&(segmentsList[segmentIdx]), cloud, 1.3);
  }

  for (size_t segmentIdx = 0; segmentIdx < segmentsList.size(); ++segmentIdx)
  {
    RemoveOutliers(&(segmentsList[segmentIdx]), cloud);
  }

  segmentsList = GetSegmentsWithMorePtsThanThreshold(segmentsList, 5);

  // Create bboxes for "thing" segments only
  for (size_t segmentIdx = 0; segmentIdx < segmentsList.size(); ++segmentIdx)
  {
    if (segmentsList[segmentIdx].isThing
        && !segmentsList[segmentIdx].ignore
        && segmentsList[segmentIdx].pointIndices.size() > 0)
    {
      objects->push_back(CreateBboxFromSegment(&segmentsList[segmentIdx], cloud));
    }
  }

  // Copy labelcloud excluding labels
  outCloud->DeepCopy(cloud);
  OverwriteCloudSegmentInfos(segmentsList, outCloud);
}


int main(int argc, char* argv[])
{
  // Parse Input ------------------------------------------------------------
  int minArgc = 8;

  if (argc < minArgc)
  {
    std::cout << "Not enough program inputs" << std::endl;
    return EXIT_FAILURE;
  }

  int firstLidarFrameToProcess = stoi(std::string(argv[1]));
  int lastLidarFrameToProcess = stoi(std::string(argv[2]));
  std::string cloudFrameSeries(argv[3]);
  std::string trajectoryFilename(argv[4]);
  std::string categoriesFilename(argv[5]);
  std::string cloudExportFolder(argv[6]);
  std::string bboxExportFolder(argv[7]);
  std::string outputAlgoName(argv[8]);
  std::string segmentsSeries(argv[9]);

  size_t nbrClouds = GetNumberOfClouds(cloudFrameSeries);

  // allow negative (python like) indexes
  if (firstLidarFrameToProcess < 0) {
      firstLidarFrameToProcess += nbrClouds;
  }
  if (lastLidarFrameToProcess < 0) {
      lastLidarFrameToProcess += nbrClouds;
  }

  // Load categories file
  CategoriesConfig catConfig(categoriesFilename.c_str());

  // Load trajectory file
  vtkSmartPointer<vtkCustomTransformInterpolator> interpolator;

  if (trajectoryFilename.size() > 0)
  {
    vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader->SetFileName(trajectoryFilename.c_str());
    reader->Update();
    vtkSmartPointer<vtkPolyData> polyTraj = reader->GetOutput();
    vtkSmartPointer<vtkTemporalTransforms> trajectory = vtkTemporalTransforms::CreateFromPolyData(polyTraj);
    interpolator = trajectory->CreateInterpolator();
    interpolator->SetInterpolationTypeToLinear();
    std::cout << "Parsed trajectory from " << trajectoryFilename << std::endl;
  }

  Json::Value files(Json::arrayValue);

  FileSeries outputCloudSeries((boost::filesystem::path(cloudExportFolder) / boost::filesystem::path(cloudFrameSeries).filename()).c_str());
  FileSeries outputBboxSeries((boost::filesystem::path(bboxExportFolder) / "detections.yml.series").c_str());

  // End Parse Input --------------------------------------------------------------------


  // Copy Frame series to output folder
  boost::filesystem::create_directory(boost::filesystem::path(cloudExportFolder));
  boost::filesystem::create_directory(boost::filesystem::path(bboxExportFolder));

  // For each lidar frame, launch the detection process
  for (size_t cloudIndex = static_cast<size_t>(firstLidarFrameToProcess); cloudIndex < static_cast<size_t>(lastLidarFrameToProcess + 1); ++cloudIndex)
  {
    std::string vtpPath = "";
    double vtpPipelineTime = 0.0; // network time, not used for projection (points have their own lidar time)
    ReadFromSeries(cloudFrameSeries, cloudIndex, vtpPath, vtpPipelineTime);
    vtkSmartPointer<vtkPolyData> cloud = ReadCloudFrame(vtpPath);

    // Load corresponding segments descriptions
    // This supposes that Cloud and segments are sync-ed, which should be as they are generated together
    std::string segmentsPath = "";
    double segmentsPipelineTime = 0.0;
    ReadFromSeries(segmentsSeries, cloudIndex, segmentsPath, segmentsPipelineTime);

    assert(vtpPipelineTime == segmentsPipelineTime);

    YAML::Node segments = YAML::LoadFile(segmentsPath)["segments_info"];

    vtkSmartPointer<vtkPolyData> outCloud = vtkSmartPointer<vtkPolyData>::New();
    std::vector<Bbox3d> objects(0);

    RefineLabelCloudWithInstances(cloud, segments, outCloud, &objects, &catConfig, interpolator);

    // Export frame cloud
    std::string outPath = "";
    GetWritePathFromSeries(cloudFrameSeries, cloudIndex, outPath, cloudExportFolder);
    WriteCloudFrame(outCloud, outPath);
    std::string cloudFilename = boost::filesystem::path(outPath).filename().string();
    outputCloudSeries.AddFile(cloudFilename, vtpPipelineTime);

    // Export bboxes as YAML
    std::string yamlFileName = (RemoveExtension(cloudFilename) + ".yml" );
    Export3DBBAsYaml(objects,
                     (boost::filesystem::path(bboxExportFolder) / yamlFileName).c_str(),
                     &catConfig,
                     outputAlgoName);
    outputBboxSeries.AddFile(yamlFileName, vtpPipelineTime);

    if ((cloudIndex - firstLidarFrameToProcess + 1) % 100 == 0)
    {
      std::cout << "Processed frames: " << (cloudIndex - firstLidarFrameToProcess + 1);
      std::cout << "/" <<(lastLidarFrameToProcess - firstLidarFrameToProcess);
      std::cout << std::endl;
    }

  }
  outputCloudSeries.WriteToFile();
  outputBboxSeries.WriteToFile();


  return EXIT_SUCCESS;
}