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


/* This Standalone tool back projects 2D segmentation labels to a 3D cloud

   Usage:
   ./install/bin/LabelCloudFromImagesDetections \
    <first frame to process>  \
    <last frame to process> \
    <segmentation type> \
    <lidar frames series> \
    <trajectory file> \
    <categories config file> \
    <output folder> \
    <timeshift> \
    <number of cameras> \
    <camera 1 images series> \
    <camera 1 segmentation root folder> \
    <camera 1 calibration file> \
    <camera 1 egovehicle mask> \
    <camera 2 images folder> \
    <camera 2 segmentation root folder> \
    <camera 2 calibration file> \
    <camera 2 egovehicle mask> \
    ...

    with:
      - first frame to process: index of the first frame to process
      - last frame to process: index of the last frame to process (can be -1)
      - segmentation type: only "panoptic" is supported for now
      - lidar frames series: full path to a frame.vtp.series file containing the names and
          times for the lidar extracted frames series
      - trajectory file: full path to a trajectory.vtp file
      - categories config file: full path to a config file as described in CategoriesConfig.h
      - output folder: full path to the folder to save the output to
      - timeshift: time shift (in seconds) between Lidar time and Network time
      - number of cameras: only 1 camera supported for now
      - camera n images series: full path to a image.jpg.series file containing the names and times
          for the image frames of camera n
      - camera n segmentation root folder: full path the the folder containing 2D segmentation values
          Expected structure:
            <camera 1 segmentation root folder>
              |_ masks
                  |_ 000001.png
                  |_ 000002.png
                  |_ ...
                  // png images with pixel values corresponding to segmentIds
                  // as in MSCoco annotations (maskPointId = r + 256 * g + 256 * 256 * b)

              |_ yamls
                  |_ 000001.yml
                  |_ 000002.yml
                  |_ ...
                  // yml storing a dictionary describing each of the segments of the image.
                  // As in MSCoco panoptic segmentation annotations, this dictionary must have a
                  // structure similar to the following example:
                  // file_name: 000001.png
                  // image_id: '000001'
                  // segments_info:
                  // - category_id: 3
                  //   id: 1
                  //   instance_id: 0
                  //   score: 0.9953383207321167
                  // - category_id: 3
                  //   id: 2
                  //   instance_id: 1

      - camera n calibration file: full path to the yaml calibration file for camera n
      - camera n egovehicle mask: full path to a grayscale image. The points which projection is on a
          non-zero value of this image are ignored.
          This can be set to "" if there is no egovehicle mask

    Output:
      The output folder <output folder> contains:
        - cloud frames corrected according to the trejectory given in <trajectory file>
        - frame.vtp.series: the corresponding series file
        - *.yml files containing the segments information for each cloud frame, with the same structure as
            the image segmentation yaml files
        - segment.yml.series: the corresponding series file

 */


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
#include "FileSystemUtils.h"

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


//------------------------------------------------------------------------------
void LabelPointsFromPanoptic(vtkSmartPointer<vtkPolyData> cloud,
                             vtkSmartPointer<vtkPolyData> labeledCloud,
                             vtkSmartPointer<vtkImageData> semanticMsk,
                             vtkSmartPointer<vtkImageData> egoVehicleMsk,
                             vtkSmartPointer<vtkImageData> img,
                             YAML::Node segments,
                             CameraModel* Model,
                             CategoriesConfig* catConfig)
{

  std::unordered_map<int, int> segmentCategoryId(segments.size());

  for (size_t i = 0; i < segments.size(); ++i)
  {
    segmentCategoryId.insert({segments[i]["id"].as<int>(), segments[i]["category_id"].as<int>()});
  }

  vtkDataArray* segmentArray = labeledCloud->GetPointData()->GetArray("segment");
  vtkDataArray* categoryArray = labeledCloud->GetPointData()->GetArray("category");

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
    else if ( (egoVehicleMsk) && (std::round(egoVehicleMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 0)) > 0))
    {
      continue;
    }
    else
    {
      // Get mask point id
      int r = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 0)));
      int g = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 1)));
      int b = static_cast<int>(std::round(semanticMsk->GetScalarComponentAsDouble(vtkCol, vtkRaw, 0, 2)));
      int maskPointId = r + 256 * g + 256 * 256 * b;
      int maskPointCategoryId = segmentCategoryId[maskPointId];

      if ((! maskPointCategoryId == 0)  && ! catConfig->IsCategoryIgnored(maskPointCategoryId) )
      {
        categoryArray->SetTuple1(pointIdx, maskPointCategoryId);
        segmentArray->SetTuple1(pointIdx, maskPointId);
      }
    }
  }
}


double CloudTimeAtImageCenter(vtkSmartPointer<vtkPolyData> cloud, CameraModel* Model)
{
  vtkDataArray* timeArray = cloud->GetPointData()->GetArray("adjustedtime");

  double minCamAzimuth = std::numeric_limits<double>::max();
  int time = timeArray->GetTuple1(0);

  auto cameraAzimuthArray = addArrayWithDefault<vtkDoubleArray, double>("cameraAzimuth", cloud, 0);

  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    double pt[3];
    cloud->GetPoint(pointIdx, pt);
    Eigen::Vector3d X(pt[0], pt[1], pt[2]);

    Eigen::Vector3d camRefX(Model->GetR().transpose() * (X - Model->GetT()));
    double cameraAzimuth = std::copysign(std::atan2(camRefX[1], camRefX[0]), camRefX[2]) - (vtkMath::Pi() / 2);
    // removing pi/2 sets 0 to the axis facing the camera
    cameraAzimuthArray->SetTuple1(pointIdx, cameraAzimuth);

    if (std::abs(cameraAzimuth) < minCamAzimuth)
    {
      time = timeArray->GetTuple1(pointIdx);
      minCamAzimuth = std::abs(cameraAzimuth);
    }
  }

  return static_cast<double>(time);
}


//------------------------------------------------------------------------------
typedef std::pair< vtkSmartPointer<vtkPolyData>, YAML::Node > result_pair;
result_pair LaunchDetectionBackProjection(vtkSmartPointer<vtkPolyData> cloud,
                                vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                CategoriesConfig* catConfig,
                                double timeshift,
                                std::string detectionsFormat, std::string imageSeriesFile,
                                std::string detectionsFolder, std::string calibFilename,
                                std::string egoVehicleMaskFilename
                                )
{
  // Get the egovehicle mask
  // cf. http://vtk.1045678.n5.nabble.com/how-to-know-wheter-an-smart-pointer-is-null-td5745505.html
  // for the further use in that function
  vtkSmartPointer<vtkJPEGReader> egoVehicleImgReader = vtkSmartPointer<vtkJPEGReader>::New();
  vtkSmartPointer<vtkImageData> egoVehicleMask;
  if (egoVehicleMaskFilename.size() > 0)
  {
    egoVehicleImgReader->SetFileName(egoVehicleMaskFilename.c_str());
    egoVehicleImgReader->Update();
    egoVehicleMask = egoVehicleImgReader->GetOutput();
  }

  // Load the calibration
  CameraModel Model;
  int ret = Model.LoadParamsFromFile(calibFilename);
  if (!ret)
  {
    std::cout << "Warning: Calibration parameters could not be read from file." << std::endl;
  }

  // Get the time of the cloud
  // Define the time of the cloud as being the middle time
  double time = 1e-6 * CloudTimeAtImageCenter(cloud, &Model) - timeshift;

  double maxTemporalDist = 1.0;
  std::pair<std::string, double> closestImgInfo = FramesSeries::GetClosestItemInSeries(imageSeriesFile, time, maxTemporalDist);
  std::string closestImgName = closestImgInfo.first;
  double closestImgTime = closestImgInfo.second;

  // Express the closest image time in the lidar time clock system
  closestImgTime = closestImgTime + timeshift;

  auto imgTimeDiffArray = addArrayWithDefault<vtkDoubleArray, double>("image2cloud_time_diff", cloud, 0);
  auto timeArray = cloud->GetPointData()->GetArray("adjustedtime");
  for (unsigned int pointIdx = 0; pointIdx < cloud->GetNumberOfPoints(); ++pointIdx)
  {
    imgTimeDiffArray->SetTuple1(pointIdx, closestImgTime - 1e-6*timeArray->GetTuple1(pointIdx) );
  }

  // Now, express the pointcloud in the vehicle reference frame corresponding
  // to the camera timestamp
  vtkSmartPointer<vtkPolyData> transformedCloud = ReferenceFrameChange(cloud, interpolator, closestImgTime);

  // initialise label cloud
  vtkSmartPointer<vtkPolyData> labeledCloud = vtkSmartPointer<vtkPolyData>::New();
  labeledCloud->DeepCopy(transformedCloud);
  auto categoryArray = addArrayWithDefault<vtkIntArray, int>("category", labeledCloud, 0);
  auto segmentArray = addArrayWithDefault<vtkIntArray, int>("segment", labeledCloud, 0);

  // load the image
  std::string imageFolder = boost::filesystem::path(imageSeriesFile).parent_path().string();
  std::string imgFilename = imageFolder + "/" + closestImgName + ".jpg";
  vtkSmartPointer<vtkJPEGReader> imgReader0 = vtkSmartPointer<vtkJPEGReader>::New();
  imgReader0->SetFileName(imgFilename.c_str());
  imgReader0->Update();
  vtkSmartPointer<vtkImageData> img = imgReader0->GetOutput();

  YAML::Node segments;

  if (detectionsFormat == "panoptic")
  {
    // load corresponding panoptic masks
    std::string labelMaskFilename = detectionsFolder + "/masks/" + closestImgName + ".png";
    vtkSmartPointer<vtkPNGReader> imgReader = vtkSmartPointer<vtkPNGReader>::New();
    imgReader->SetFileName(labelMaskFilename.c_str());
    imgReader->Update();
    vtkSmartPointer<vtkImageData> labelMask = imgReader->GetOutput();

    // load corresponding detections descriptions
    std::string segmentsFilename = detectionsFolder + "/yamls/" + closestImgName + ".yml";
    segments = YAML::LoadFile(segmentsFilename);

    LabelPointsFromPanoptic(transformedCloud, labeledCloud, labelMask, egoVehicleMask, img, segments["segments_info"], &Model, catConfig);
   }
  else
  {
    std::cout<<"Unknown detections format " << detectionsFolder << ": No boxes computed."<<std::endl;
  }

  return result_pair(labeledCloud, segments);
}

//------------------------------------------------------------------------------
// negative numbers (python like indexes) are accepted for {first,last}LidarFrameToProcess
// so to run on all frames pass respectively 0 and -1
int main(int argc, char* argv[])
{
  // Setup implemented detection formats
  std::vector<std::string> allowedDetectionsFormats(0);

  allowedDetectionsFormats.push_back("panoptic");

  // Parse Input ---------------------------------------------------------------------
  int minArgc = 10;
  int camSpecificArgc = 4;

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
  std::string exportFolder(argv[7]);
  double timeshift = std::atof(argv[8]);
  unsigned int nbrCameras = static_cast<unsigned int>(std::atoi(argv[9]));

  if (nbrCameras > 1)
  {
    std::cout << " Only single camera is supported for now (provided: NbrCameras = " << nbrCameras << ")" << std::endl;
    return EXIT_FAILURE;
  }

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
  std::vector<std::string> imageSeriesFiles(0);
  std::vector<std::string> detectionsFolders(0);
  std::vector<std::string> calibFilenames(0);
  std::vector<std::string> egoVehicleMaskFilenames(0);
  for (unsigned int cameraIndex = 0; cameraIndex < nbrCameras; ++cameraIndex)
  {
    imageSeriesFiles.push_back(std::string(argv[minArgc + cameraIndex]));
    detectionsFolders.push_back(std::string(argv[minArgc + nbrCameras + cameraIndex]));
    calibFilenames.push_back(std::string(argv[minArgc + 2 * nbrCameras + cameraIndex]));
    egoVehicleMaskFilenames.push_back(std::string(argv[minArgc + 3 * nbrCameras + cameraIndex]));


    std::cout << cameraIndex << std::endl;
    std::cout << std::string(argv[minArgc + cameraIndex]) << std::endl;
    std::cout << "imgs --------------------------------------------------" << std::endl;
    std::cout << std::string(argv[minArgc + nbrCameras + cameraIndex]) << std::endl;
    std::cout << "detections --------------------------------------------" << std::endl;
    std::cout << std::string(argv[minArgc + 2 * nbrCameras + cameraIndex]) << std::endl;
    std::cout << "egovehicle mask ---------------------------------------" << std::endl;
    std::cout << std::string(argv[minArgc + 3 * nbrCameras + cameraIndex]) << std::endl;
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

  FileSeries outputCloudSeries((boost::filesystem::path(exportFolder) / boost::filesystem::path(cloudFrameSeries).filename()).string());
  FileSeries outputSegmentSeries((boost::filesystem::path(exportFolder) / "segment.yml.series").string());

  // End Parse Input --------------------------------------------------------------------

  // Copy Frame series to output folder
  boost::filesystem::create_directory(boost::filesystem::path(exportFolder) );

  // For each lidar frame, launch the detection process
  for (size_t cloudIndex = static_cast<size_t>(firstLidarFrameToProcess); cloudIndex < static_cast<size_t>(lastLidarFrameToProcess + 1); ++cloudIndex)
  {
    // read cloud
    std::string vtpPath = "";
    double vtpPipelineTime = 0.0; // network time, not used for projection (points have their own lidar time)
    FramesSeries::ReadFromSeries(cloudFrameSeries, cloudIndex, vtpPath, vtpPipelineTime);
    vtkSmartPointer<vtkPolyData> cloud = FramesSeries::ReadCloudFrame(vtpPath);

    YAML::Node segments;
    vtkSmartPointer<vtkPolyData> labeledCloud;
    // for each image, launch detection back projection
    for (unsigned int cameraIndex = 0; cameraIndex < imageSeriesFiles.size(); ++cameraIndex)
    {
      // Temporary hack to work on only 1 camera (to change when changing dataset)
      if (true) // HACK: Should work with multiple cameras but works only with one
      {
        result_pair results = LaunchDetectionBackProjection(cloud, interpolator, &catConfig, timeshift, detectionsFormat,
                                                     imageSeriesFiles[cameraIndex], detectionsFolders[cameraIndex],
                                                     calibFilenames[cameraIndex],
                                                     egoVehicleMaskFilenames[cameraIndex]);
        labeledCloud = results.first;
        segments = results.second;
      }
    }

    // Export frame cloud
    std::string outPath = "";
    FramesSeries::GetWritePathFromSeries(cloudFrameSeries, cloudIndex, outPath, exportFolder);
    FramesSeries::WriteCloudFrame(labeledCloud, outPath);
    std::string cloudFilename = boost::filesystem::path(outPath).filename().string();
    outputCloudSeries.AddFile(cloudFilename, vtpPipelineTime);

    // Export segments infos
    segments["file_name"] = cloudFilename;
    std::string segmentsFileName = (RemoveExtension(cloudFilename) + ".yml" );
    std::ofstream fout((boost::filesystem::path(exportFolder) / segmentsFileName).c_str());
    fout << segments;
    fout.close();

    outputSegmentSeries.AddFile(segmentsFileName, vtpPipelineTime);
  }
  outputCloudSeries.WriteToFile();
  outputSegmentSeries.WriteToFile();

  return EXIT_SUCCESS;
}
