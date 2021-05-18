//=========================================================================
//
// Copyright 2019 Kitware, Inc.
// Author: Guilbert Pierre (spguilbert@gmail.com)
// Date: 08-02-2019
//
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

// LOCAL
#include "vtkPCLConversions.h"
#include "vtkTemporalTransforms.h"
#include "Common/vtkCustomTransformInterpolator.h"
#include "CameraModel.h"
#include "vtkBoundingBoxReader.h"
#include "vtkBoundingBox.h"
#include "CameraProjection.h"
#include "Common/BoundingBox.h"
#include "vtkEigenTools.h"
#include "CategoriesConfig.h"
#include "FramesSeriesUtils.h"

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
#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkTransform.h>
#include <vtkJPEGReader.h>
#include <vtkPNGReader.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkFieldData.h>
#include <vtk_jsoncpp.h>

class SemanticCentroid
{
public:
  Eigen::Vector3d center;
  int class_id;
  std::string type;
};


//------------------------------------------------------------------------------
std::vector<OrientedBoundingBox<2>> Create2DBBFromPolyData(vtkSmartPointer<vtkMultiBlockDataSet> vtkBBs)
{
  std::vector<OrientedBoundingBox<2>> bbList;
  for (unsigned int bbIdx = 0; bbIdx < vtkBBs->GetNumberOfBlocks(); ++bbIdx)
  {
    vtkSmartPointer<vtkPolyData> vtkBB = vtkPolyData::SafeDownCast(vtkBBs->GetBlock(bbIdx));
    double minCorner[3], maxCorner[3];
    vtkBB->GetPoint(0, minCorner);
    vtkBB->GetPoint(2, maxCorner);

    OrientedBoundingBox<2> bb(Eigen::Vector2d(minCorner[0], minCorner[1]), Eigen::Vector2d(maxCorner[0], maxCorner[1]));
    bb.Type = vtkBB->GetFieldData()->GetAbstractArray(0)->GetVariantValue(0).ToString();
    bbList.push_back(bb);
  }
  return bbList;
}

//------------------------------------------------------------------------------
std::vector<Eigen::VectorXd> RemovePercentilFarthest(std::vector<Eigen::VectorXd> inputList, double percentil)
{
  std::vector<Eigen::VectorXd> closestPoints(0);
  std::vector<std::pair<double, size_t>> toSortWithDist(inputList.size());
  for (size_t i = 0; i < inputList.size(); ++i)
  {
    toSortWithDist[i].first = inputList[i].norm();
    toSortWithDist[i].second = i;
  }

  std::sort(toSortWithDist.begin(), toSortWithDist.end());

  size_t maxIndex = static_cast<size_t>(std::ceil(static_cast<double>(inputList.size() - 1) * percentil));
  for (size_t i = 0; i <= maxIndex; ++i)
  {
    closestPoints.push_back(inputList[toSortWithDist[i].second]);
  }

  return closestPoints;
}

//------------------------------------------------------------------------------
bool CheckBBSemMaskConsistency(int categoryId, int r, int g, int b,
                               CategoriesConfig* catConfig)
{
  std::vector<double> categoryColor = catConfig->GetCategoryColor(categoryId);
  bool consistency = (b == categoryColor[0]) && (g == categoryColor[1]) && (r == categoryColor[2]);

  return consistency;
}

//------------------------------------------------------------------------------
std::vector<bool> ComputeBBSegMaskConsistency(std::vector<OrientedBoundingBox<2>> bbList,
                                              vtkSmartPointer<vtkImageData> semanticMsk,
                                              CategoriesConfig* catConfig)
{
  std::vector<bool> isConsistent(bbList.size(), true);
  for (size_t bbIdx = 0; bbIdx < bbList.size(); ++bbIdx)
  {
    double pixelCount = 0;
    double pixelConsistentCount = 0;
    for (int col = 0; col < semanticMsk->GetDimensions()[0]; ++col)
    {
      for (int row = 0; row < semanticMsk->GetDimensions()[1]; ++row)
      {
        if (bbList[bbIdx].IsPointInside(Eigen::Vector2d(col, row)))
        {
          pixelCount += 1.0;
          int r = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(col, row, 0, 0)));
          int g = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(col, row, 0, 1)));
          int b = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(col, row, 0, 2)));

          int categoryId = catConfig->GetCategoryIdFromExternalName(bbList[bbIdx].Type, "yolo");
          if (CheckBBSemMaskConsistency(categoryId, r, g, b, catConfig))
          {
            pixelConsistentCount += 1.0;
          }
        }
      }
    }

    // compute area fraction of consistent pixel
    double consistentFraction = pixelConsistentCount / pixelCount;
    if (consistentFraction < 0.175)
    {
      isConsistent[bbIdx] = false;
    }
  }
  return isConsistent;
}

//------------------------------------------------------------------------------
std::vector<SemanticCentroid> DetectAndComputeCentroidFromBBoxPlusSemantic(vtkSmartPointer<vtkPolyData> cloud,
                                                       vtkSmartPointer<vtkImageData> semanticMsk,
                                                       vtkSmartPointer<vtkImageData> img,
                                                       vtkSmartPointer<vtkMultiBlockDataSet> bbs,
                                                       CameraModel* Model,
                                                       CategoriesConfig* catConfig,
                                                       std::string bboxLabelKey)
{
  assert(Model != nullptr);

  // Convert the polydata to 2D boundingbox structure
  std::vector<OrientedBoundingBox<2>> bbList = Create2DBBFromPolyData(bbs);
  std::vector<std::vector<Eigen::VectorXd>> pointsInBB(bbList.size());

  // Discard a potential bounding box if there is no
  // consistency between segmentation and bboxes
  std::vector<bool> isBBConsistent = ComputeBBSegMaskConsistency(bbList, semanticMsk, catConfig);

  // loop over points of the pointcloud
  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    double pt[3];
    cloud->GetPoint(pointIdx, pt);
    Eigen::Vector3d X(pt[0], pt[1], pt[2]);

    // Project the 3D point onto the image
    Eigen::Vector2d y = Model->Projection(X, true);

    // y represents the pixel coordinates using opencv convention, we need to
    // go back to vtkImageData pixel convention
    int vtkRaw = static_cast<int>(std::round(y(1)));
    int vtkCol = static_cast<int>(std::round(y(0)));

    // Check if the projected point is in the region of interest of the image
    if ((vtkRaw < 0) || (vtkRaw >= img->GetDimensions()[1]) ||
        (vtkCol < 0) || (vtkCol >= img->GetDimensions()[0]))
    {
      continue;
    }

    // loop over the bounding box
    for (unsigned int bbIdx = 0; bbIdx < bbList.size(); ++bbIdx)
    {
      OrientedBoundingBox<2> bb = bbList[bbIdx];
      std::string type = bb.Type;
      int categoryId = catConfig->GetCategoryIdFromExternalName(type, bboxLabelKey);
      bool bbToIgnore = catConfig->IsCategoryIgnored(categoryId);

      // reject over kind of object
      if ( !bbToIgnore )
      {
        // check that the projected point is inside the bounding box
        if (bb.IsPointInside(y) && isBBConsistent[bbIdx])
        {
          // if it is, we still need to check the consistency with
          // the semantic segmentation
          int r = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 0)));
          int g = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 1)));
          int b = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 2)));

          if (CheckBBSemMaskConsistency(categoryId, r, g, b, catConfig))
          {
            // Add the 3D points to the list since it is in the bb frustrum
            pointsInBB[bbIdx].push_back(X);
          }
        }
      }
    }
  }

  // compute the centroid
  std::vector<SemanticCentroid> positions;
  for (unsigned int objectIdx = 0; objectIdx < pointsInBB.size(); ++objectIdx)
  {
    if (pointsInBB[objectIdx].size() > 0)
    {
      // first, only keep the 50 percent points closest to the camera center
      std::vector<Eigen::VectorXd> closestPoints = RemovePercentilFarthest(pointsInBB[objectIdx], 0.30);

      // Then compute the centroid of the remaining points
      SemanticCentroid centroid;
      centroid.center = MultivariateMedian(closestPoints, 1e-6);
      int categoryId = catConfig->GetCategoryIdFromExternalName(bbList[objectIdx].Type, bboxLabelKey);
      centroid.type = catConfig->GetCategoryName(categoryId);
      positions.push_back(centroid);
      std::cout << "Centroid of " << centroid.type << " is: " << centroid.center.transpose() << std::endl;
    }
  }

  return positions;
}

//------------------------------------------------------------------------------
std::vector<SemanticCentroid> DetectAndComputeCentroidFromPanoptic(vtkSmartPointer<vtkPolyData> cloud,
                                                       vtkSmartPointer<vtkImageData> semanticMsk,
                                                       vtkSmartPointer<vtkImageData> img,
                                                       YAML::Node segments,
                                                       CameraModel* Model,
                                                       CategoriesConfig* catConfig)
{
  std::vector<std::vector<Eigen::VectorXd>> pointsInSegment(segments.size());
  std::vector<std::string> segmentType(segments.size());
  std::vector<bool> segmentToIgnore(segments.size());
  for (size_t i = 0; i < segments.size(); ++i)
  {
    int segmentCategoryId = segments[i]["category_id"].as<int>();
    segmentToIgnore[i] = catConfig->IsCategoryIgnored(segmentCategoryId);
    segmentType[i] = catConfig->GetCategoryName(segmentCategoryId);
  }

  // loop over points of the pointcloud
  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    double pt[3];
    cloud->GetPoint(pointIdx, pt);
    Eigen::Vector3d X(pt[0], pt[1], pt[2]);

    // Project the 3D point onto the image
    Eigen::Vector2d y = Model->Projection(X, true);

    // y represents the pixel coordinates using opencv convention, we need to
    // go back to vtkImageData pixel convention
    int vtkRaw = static_cast<int>(std::round(y(1)));
    int vtkCol = static_cast<int>(std::round(y(0)));

    // Check if the projected point is in the region of interest of the image
    if ((vtkRaw < 0) || (vtkRaw >= img->GetDimensions()[1]) ||
        (vtkCol < 0) || (vtkCol >= img->GetDimensions()[0]))
    {
      continue;
    }

    // Get mask point id
    int r = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 0)));
    int g = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 1)));
    int b = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 2)));
    int maskPointId = r + 256 * g + 256 * 256 * b;

    // loop over the segments
    for (unsigned int segmentIdx = 0; segmentIdx < segments.size(); ++segmentIdx)
    {
      if (maskPointId == segments[segmentIdx]["id"].as<int>() && !segmentToIgnore[segmentIdx] )
      {
        // Add the 3D points to the list since it is in the bb frustrum
        pointsInSegment[segmentIdx].push_back(X);
        break;
      }
    }
  }

  // compute the centroid
  std::vector<SemanticCentroid> positions;
  for (unsigned int objectIdx = 0; objectIdx < pointsInSegment.size(); ++objectIdx)
  {
    if (pointsInSegment[objectIdx].size() > 0)
    {
      // first, only keep the 50 percent points closest to the camera center
      std::vector<Eigen::VectorXd> closestPoints = RemovePercentilFarthest(pointsInSegment[objectIdx], 0.30);

      // Then compute the centroid of the remaining points
      SemanticCentroid centroid;
      centroid.center = MultivariateMedian(closestPoints, 1e-6);
      centroid.type = segmentType[objectIdx];
      positions.push_back(centroid);
      std::cout << "Centroid of " << centroid.type << " is: " << centroid.center.transpose() << std::endl;
    }
  }

  return positions;
}


//------------------------------------------------------------------------------
std::vector<SemanticCentroid> LaunchDetectionBackProjection(vtkSmartPointer<vtkPolyData> cloud,
                                vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                CategoriesConfig* catConfig,
                                double timeshift,
                                std::string detectionsFormat, std::string imageFolder,
                                std::string detectionsFolder, std::string calibFilename)
{
  // Get the time of the cloud
  // Define the time of the cloud as being the middle time
  vtkDataArray* timeArray = cloud->GetPointData()->GetArray("adjustedtime");
  double time = 1e-6 * static_cast<double>((timeArray->GetTuple1(0) + timeArray->GetTuple1(cloud->GetNumberOfPoints() - 1))) / 2.0 - timeshift;

  // Get the name of the temporally closest image
  std::string filename = imageFolder + "/image.jpg.series";

  double maxTemporalDist = 1.0;
  std::pair<std::string, double> closestImgInfo = FramesSeries::GetClosestItemInSeries(filename, time, maxTemporalDist);
  std::string closestImgName = closestImgInfo.first;
  double closestImgTime = closestImgInfo.second;

  // Express the closest image time in the lidar time clock system
  closestImgTime = closestImgTime + timeshift;

  // Now, express the pointcloud in the vehicle reference frame corresponding
  // to the camera timestamp
  vtkSmartPointer<vtkPolyData> transformedCloud = ReferenceFrameChange(cloud, interpolator, closestImgTime);

  // Load the calibration
  CameraModel Model;
  int ret = Model.LoadParamsFromFile(calibFilename);
  if (!ret)
  {
    std::cout << "Warning: Calibration parameters could not be read from file." << std::endl;
  }

  // load the image
  std::string imgFilename = imageFolder + "/" + closestImgName + ".jpg";
  vtkSmartPointer<vtkJPEGReader> imgReader0 = vtkSmartPointer<vtkJPEGReader>::New();
  imgReader0->SetFileName(imgFilename.c_str());
  imgReader0->Update();
  vtkSmartPointer<vtkImageData> img = imgReader0->GetOutput();

  std::vector<SemanticCentroid> results(0);
  if (detectionsFormat == "yolo+semantic")
  {
    // load corresponding semantic masks
    std::string maskFilename = detectionsFolder + "/semantic_masks/" + closestImgName + ".png";
    vtkSmartPointer<vtkPNGReader> imgReader = vtkSmartPointer<vtkPNGReader>::New();
    imgReader->SetFileName(maskFilename.c_str());
    imgReader->Update();
    vtkSmartPointer<vtkImageData> semanticMask = imgReader->GetOutput();

    // load corresponding detections bboxes
    std::string bboxesFilename = detectionsFolder + "/bboxes/" + closestImgName + ".yml";
    vtkSmartPointer<vtkBoundingBoxReader> bbReader = vtkSmartPointer<vtkBoundingBoxReader>::New();
    bbReader->SetFileName(bboxesFilename);
    bbReader->SetImageHeight(img->GetDimensions()[1]);
    bbReader->Update();
    vtkSmartPointer<vtkMultiBlockDataSet> bbs = bbReader->GetOutput();

    // Launch 3D median center computation
    results = DetectAndComputeCentroidFromBBoxPlusSemantic(transformedCloud, semanticMask,
                                                           img, bbs, &Model, catConfig, "yolo");
  }
  else if (detectionsFormat == "panoptic")
  {
    // load corresponding panoptic masks
    std::string maskFilename = detectionsFolder + "/masks/" + closestImgName + ".png";
    vtkSmartPointer<vtkPNGReader> imgReader = vtkSmartPointer<vtkPNGReader>::New();
    imgReader->SetFileName(maskFilename.c_str());
    imgReader->Update();
    vtkSmartPointer<vtkImageData> mask = imgReader->GetOutput();

    // load corresponding detections desriptions
    std::string segmentsFilename = detectionsFolder + "/yamls/" + closestImgName + ".yml";
    YAML::Node segments = YAML::LoadFile(segmentsFilename)["segments_info"];


    // Launch 3D median center computation
    results = DetectAndComputeCentroidFromPanoptic(transformedCloud, mask, img, segments, &Model, catConfig);
  }
  else
  {
    std::cout<<"Unknown detections format "<<detectionsFolder<<": No boxes computed."<<std::endl;
  }

  return results;
}

//------------------------------------------------------------------------------
void Export3DBBAsYaml(std::vector<std::vector<SemanticCentroid>> objects,
                      std::string outputYamlFile, CategoriesConfig* catConfig,
                      std::string algo_name)
{
  YAML::Node ymlFile;
  ymlFile["meta"] = YAML::Node();
  ymlFile["objects"] = YAML::Node();

  ymlFile["meta"]["date"] = std::string("N.A.");
  ymlFile["meta"]["source"] = std::string("N.A.");

  std::vector<SemanticCentroid> allObjects;
  for (size_t i = 0; i < objects.size(); ++i)
  {
    for (size_t j = 0; j < objects[i].size(); ++j)
    {
      allObjects.push_back(objects[i][j]);
    }
  }

  for (size_t i = 0; i < allObjects.size(); ++i)
  {
    YAML::Node currentBB;

    int categoryId = catConfig->GetCategoryIdFromName(allObjects[i].type);
    currentBB["label"] = catConfig->GetSuperCategory(categoryId);

    currentBB["custom"] = YAML::Node();
    currentBB["custom"]["confidence"] = 271828;
    currentBB["custom"]["algo"] = algo_name;

    currentBB["selector"] = YAML::Node();
    currentBB["selector"]["center"].push_back(allObjects[i].center(0));
    currentBB["selector"]["center"].push_back(allObjects[i].center(1));
    currentBB["selector"]["center"].push_back(allObjects[i].center(2));

    currentBB["selector"]["dimensions"].push_back(2.0);
    currentBB["selector"]["dimensions"].push_back(2.0);
    currentBB["selector"]["dimensions"].push_back(2.0);

    currentBB["selector"]["type"] = "3D bounding box";

    ymlFile["objects"].push_back(currentBB);
  }

  std::ofstream fout(outputYamlFile.c_str());
  fout << ymlFile;
}

//------------------------------------------------------------------------------
// negative numbers (python like indexes) are accepted for {first,last}LidarFrameToProcess
// so to run on all frames pass respectively 0 and -1
int main(int argc, char* argv[])
{
  // Setup implemented detection formats
  std::vector<std::string> allowedDetectionsFormats(0);
  allowedDetectionsFormats.push_back("yolo+semantic");
  allowedDetectionsFormats.push_back("panoptic");

  int minArgc = 11;
  int camSpecificArgc = 3;
  // Check if there is the minimal number of inputs
  if (argc < minArgc)
  {
    std::cout << "Not enough program inputs" << std::endl;
    return EXIT_FAILURE;
  }

  int firstLidarFrameToProcess = stoi(std::string(argv[1]));
  int lastLidarFrameToProcess = stoi(std::string(argv[2]));
  std::string detectionsFormat = std::string(argv[3]);
  std::string cloudFrameSeries(argv[4]);
  std::string trajectoryFilename(argv[5]);
  std::string categoriesFilename(argv[6]);
  std::string export3DBBFolder(argv[7]);
  std::string outputAlgoName(argv[8]);
  double timeshift = std::atof(argv[9]);
  unsigned int nbrCameras = static_cast<unsigned int>(std::atoi(argv[10]));

  // Check if detection type is supported
  if (std::find(begin(allowedDetectionsFormats), end(allowedDetectionsFormats), detectionsFormat)
        == end(allowedDetectionsFormats))
  {
    std::cout << "Unsupported detections format" << std::endl;
    std::cout << "Got: " << detectionsFormat << std::endl;
    std::cout << "implemented: ";
    for (size_t i = 0; i < allowedDetectionsFormats.size(); ++i)
    {
      std::cout << allowedDetectionsFormats[i] << " ";
    }
    std::cout << std::endl;

    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Using detections format " << detectionsFormat << std::endl;
  }

  // Check if there is the expected number of inputs
  unsigned int expectedNbrInput = minArgc + nbrCameras * camSpecificArgc;
  if (static_cast<unsigned int>(argc) != expectedNbrInput)
  {
    std::cout << "Unexpected nbr of inputs" << std::endl;
    std::cout << "Got: " << argc << " inputs" << std::endl;
    std::cout << "expected: " << expectedNbrInput << std::endl;
    return EXIT_FAILURE;
  }

  // Get the folder of the images and detections
  std::vector<std::string> imageFolders(0);
  std::vector<std::string> detectionsFolders(0);
  std::vector<std::string> calibFilenames(0);
  for (unsigned int cameraIndex = 0; cameraIndex < nbrCameras; ++cameraIndex)
  {
    imageFolders.push_back(std::string(argv[minArgc + cameraIndex]));
    detectionsFolders.push_back(std::string(argv[minArgc + nbrCameras + cameraIndex]));
    calibFilenames.push_back(std::string(argv[minArgc + 2 * nbrCameras + cameraIndex]));

    std::cout << cameraIndex << std::endl;
    std::cout << std::string(argv[minArgc + cameraIndex]) << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << std::string(argv[minArgc + nbrCameras + cameraIndex]) << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << std::string(argv[minArgc + 2 * nbrCameras + cameraIndex]) << std::endl;
    std::cout << std::endl;
  }

  // Load categories file

  CategoriesConfig catConfig(categoriesFilename.c_str());

  // Load trajectory file
  vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName(trajectoryFilename.c_str());
  reader->Update();
  vtkSmartPointer<vtkPolyData> polyTraj = reader->GetOutput();
  vtkSmartPointer<vtkTemporalTransforms> trajectory = vtkTemporalTransforms::CreateFromPolyData(polyTraj);
  vtkSmartPointer<vtkCustomTransformInterpolator> interpolator = trajectory->CreateInterpolator();
  interpolator->SetInterpolationTypeToLinear();

  size_t nbrClouds = FramesSeries::GetNumberOfClouds(cloudFrameSeries);

  // allow negative (python like) indexes
  if (firstLidarFrameToProcess < 0) {
      firstLidarFrameToProcess += nbrClouds;
  }
  if (lastLidarFrameToProcess < 0) {
      lastLidarFrameToProcess += nbrClouds;
  }

  Json::Value outputSeries;
  Json::Value files(Json::arrayValue);

  // For each lidar frame, launch the detection and tracking process
  for (size_t cloudIndex = static_cast<size_t>(firstLidarFrameToProcess); cloudIndex < static_cast<size_t>(lastLidarFrameToProcess + 1); ++cloudIndex)
  {
    // read cloud
    std::string vtpPath = "";
    double vtpPipelineTime = 0.0; // network time, not used for projection (points have their own lidar time)
    FramesSeries::ReadFromSeries(cloudFrameSeries, cloudIndex, vtpPath, vtpPipelineTime);
    vtkSmartPointer<vtkPolyData> cloud = FramesSeries::ReadCloudFrame(vtpPath);

    // object detected
    std::vector<std::vector<SemanticCentroid>> objects;

    // for each image, launch detection back projection
    for (unsigned int cameraIndex = 0; cameraIndex < imageFolders.size(); ++cameraIndex)
    {
      std::vector<SemanticCentroid> currentCamObjects = LaunchDetectionBackProjection(cloud, interpolator, &catConfig,
                                                                                      timeshift, detectionsFormat,
                                                                                      imageFolders[cameraIndex],
                                                                                      detectionsFolders[cameraIndex],
                                                                                      calibFilenames[cameraIndex]);
      std::cout << "Added: " << currentCamObjects.size() << " objects for camera: " << cameraIndex << std::endl;
      objects.push_back(currentCamObjects);
    }

    // Export as yaml
    std::string yamlOutput = (boost::filesystem::path(export3DBBFolder) / boost::filesystem::path(vtpPath).stem()).string() + ".yml";
    Export3DBBAsYaml(objects, yamlOutput, &catConfig, outputAlgoName);
    //std::vector<std::string>["files"] = YAML::NodeType.
    Json::Value node;
    node["name"] = boost::filesystem::path(yamlOutput).filename().string();
    node["time"] = vtpPipelineTime;
    files.append(node);
  }

  outputSeries["files"] = files;
  outputSeries["file-series-version"] = "1.0";

  std::ofstream outputSeriesStream((boost::filesystem::path(export3DBBFolder) / boost::filesystem::path("detections.yml.series")).c_str());
  outputSeriesStream << outputSeries;
  outputSeriesStream.close();

  return EXIT_SUCCESS;
}
