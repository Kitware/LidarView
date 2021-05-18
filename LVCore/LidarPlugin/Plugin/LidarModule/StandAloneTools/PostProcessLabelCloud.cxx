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
#include "Common/vtkCustomTransformInterpolator.h"
#include "vtkBoundingBoxReader.h"
#include "CameraProjection.h"
#include "Common/BoundingBox.h"
#include "vtkEigenTools.h"
#include "CategoriesConfig.h"
#include "FramesSeriesUtils.h"
#include "FileSystemUtils.h"
#include "SegmentedCloudTransformations.h"

// STD
#include <iostream>

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
#include <vtkDoubleArray.h>
#include <vtkMath.h>


// Bbox utils -----------------------------------------------------------------
Eigen::Matrix3d MaybeGetRotationMatrixFromInterpolator(vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                                       Eigen::Matrix3d R,
                                                       double time,
                                                       double timeSpan=1)
{
  std::pair<Eigen::Matrix3d, Eigen::Vector3d> H0 = GetRTFromTime(interpolator, time - timeSpan/2);
  std::pair<Eigen::Matrix3d, Eigen::Vector3d> H1 = GetRTFromTime(interpolator, time + timeSpan/2);
  Eigen::Vector3d heading = H1.second - H0.second;
  if (heading.norm() >=1)
  {
    double yaw = std::atan2(heading[1], heading[0]);
    double pitch = -std::asin(heading[2]/ heading.norm());
    double roll = 0;
    R = RollPitchYawToMatrix(roll, pitch, yaw);
  }
  else
  {
    std::cout << "Rotation matrix not updated as the displacement was too small (" << heading.norm() <<"). ";
    std::cout << "Consider increasing timespan" << std::endl;
  }
  return R;
}


void Export3DBBAsYaml(std::vector<OrientedBoundingBox<3>> objects,
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

    currentBB["label"] = catConfig->GetSuperCategory(objects[i].ClassId);

    currentBB["custom"] = YAML::Node();
    currentBB["custom"]["confidence"] = objects[i].Confidence;
    currentBB["custom"]["algo"] = algo_name;
    currentBB["custom"]["adjustedtime"] = objects[i].TimeStamp;
    currentBB["custom"]["class_id"] = objects[i].ClassId;
    currentBB["custom"]["segment_id"] = objects[i].SegmentId;

    currentBB["selector"] = YAML::Node();
    currentBB["selector"]["center"].push_back(objects[i].Center(0));
    currentBB["selector"]["center"].push_back(objects[i].Center(1));
    currentBB["selector"]["center"].push_back(objects[i].Center(2));

    currentBB["selector"]["dimensions"].push_back(objects[i].Width(0));
    currentBB["selector"]["dimensions"].push_back(objects[i].Width(1));
    currentBB["selector"]["dimensions"].push_back(objects[i].Width(2));

    Eigen::Vector3d rpy = 180.0 / vtkMath::Pi() * MatrixToRollPitchYaw(objects[i].Orientation);
    currentBB["selector"]["rotation"].push_back(rpy(0));
    currentBB["selector"]["rotation"].push_back(rpy(1));
    currentBB["selector"]["rotation"].push_back(rpy(2));

    currentBB["selector"]["type"] = "3D bounding box";

    ymlFile["objects"].push_back(currentBB);
  }

  std::ofstream fout(outputYamlFile.c_str());
  fout << ymlFile;
  fout.close();
}


// ----------------------------------------------------------------------------

void RefineLabelCloudWithInstances(vtkSmartPointer<vtkPolyData> cloud,
                                   YAML::Node yamlSegments,
                                   vtkSmartPointer<vtkPolyData> outCloud,  // Will store refined Cloud
                                   std::vector<OrientedBoundingBox<3>>* objects,   // Will store instances 3D Bboxes
                                   CategoriesConfig* catConfig,
                                   Eigen::Matrix3d BboxesRotation=Eigen::Matrix3d::Identity())
{
  SegmentedCloud segmentedCloud(cloud, yamlSegments, catConfig);

  int nbNeighbors = 10;
  double maxDistance = 0.3; // in meters
  segmentedCloud.ApplyKNNsToSegments(nbNeighbors, maxDistance);
  segmentedCloud.ApplyDistanceThreshold(1.3);
  segmentedCloud.RemoveSegmentsOutliers();
  segmentedCloud.RemoveSegmentsWithLessPointsThanThreshold(5);
  auto bboxes = segmentedCloud.CreateBoundingBoxes(BboxesRotation);
  segmentedCloud.UpdateCloud();
  outCloud->DeepCopy(segmentedCloud.GetPointCloud());

  for (auto bb: bboxes)
  {
    objects->push_back(bb);
  }
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

  size_t nbrClouds = FramesSeries::GetNumberOfClouds(cloudFrameSeries);

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
  vtkSmartPointer<vtkTemporalTransforms> trajectory;
  if (trajectoryFilename.size() > 0)
  {
    vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader->SetFileName(trajectoryFilename.c_str());
    reader->Update();
    vtkSmartPointer<vtkPolyData> polyTraj = reader->GetOutput();
    trajectory = vtkTemporalTransforms::CreateFromPolyData(polyTraj);
    interpolator = trajectory->CreateInterpolator();
    interpolator->SetInterpolationTypeToLinear();
    std::cout << "Parsed trajectory from " << trajectoryFilename << std::endl;
  }

  Json::Value files(Json::arrayValue);

  FileSeries outputCloudSeries((boost::filesystem::path(cloudExportFolder) / boost::filesystem::path(cloudFrameSeries).filename()).string());
  FileSeries outputBboxSeries((boost::filesystem::path(bboxExportFolder) / "detections.yml.series").string());

  // End Parse Input --------------------------------------------------------------------


  // Copy Frame series to output folder
  boost::filesystem::create_directory(boost::filesystem::path(cloudExportFolder));
  boost::filesystem::create_directory(boost::filesystem::path(bboxExportFolder));

  Eigen::Matrix3d RotFromTrajectoryHeading = Eigen::Matrix3d::Identity();

  // For each lidar frame, launch the detection process
  for (size_t cloudIndex = static_cast<size_t>(firstLidarFrameToProcess); cloudIndex < static_cast<size_t>(lastLidarFrameToProcess + 1); ++cloudIndex)
  {
    std::string vtpPath = "";
    double vtpPipelineTime = 0.0; // network time, not used for projection (points have their own lidar time)
    FramesSeries::ReadFromSeries(cloudFrameSeries, cloudIndex, vtpPath, vtpPipelineTime);
    vtkSmartPointer<vtkPolyData> cloud = FramesSeries::ReadCloudFrame(vtpPath);

    // Load corresponding segments descriptions
    // This supposes that Cloud and segments are sync-ed, which should be as they are generated together
    std::string segmentsPath = "";
    double segmentsPipelineTime = 0.0;
    FramesSeries::ReadFromSeries(segmentsSeries, cloudIndex, segmentsPath, segmentsPipelineTime);

    assert(vtpPipelineTime == segmentsPipelineTime);

    YAML::Node segments = YAML::LoadFile(segmentsPath)["segments_info"];

    if (interpolator)
    {
      vtkDataArray* timeArray = cloud->GetPointData()->GetArray("adjustedtime");
      double time = 1e-6 * static_cast<double>((timeArray->GetTuple1(0) + timeArray->GetTuple1(cloud->GetNumberOfPoints() - 1))) / 2.0;

      RotFromTrajectoryHeading = MaybeGetRotationMatrixFromInterpolator(interpolator, RotFromTrajectoryHeading, time, 1);
    }

    vtkSmartPointer<vtkPolyData> outCloud = vtkSmartPointer<vtkPolyData>::New();
    std::vector<OrientedBoundingBox<3>> objects(0);

    RefineLabelCloudWithInstances(cloud, segments, outCloud, &objects, &catConfig, RotFromTrajectoryHeading);

    // Export frame cloud
    std::string outPath = "";
    FramesSeries::GetWritePathFromSeries(cloudFrameSeries, cloudIndex, outPath, cloudExportFolder);
    FramesSeries::WriteCloudFrame(outCloud, outPath);
    std::string cloudFilename = boost::filesystem::path(outPath).filename().string();
    outputCloudSeries.AddFile(cloudFilename, vtpPipelineTime);

    // Export bboxes as YAML
    std::string yamlFileName = (RemoveExtension(cloudFilename) + ".yml" );
    Export3DBBAsYaml(objects,
                     (boost::filesystem::path(bboxExportFolder) / yamlFileName).string(),
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
