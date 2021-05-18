// LOCAL
#include "vtkTemporalTransforms.h"
#include "Common/vtkCustomTransformInterpolator.h"
#include "CameraModel.h"
#include "vtkEigenTools.h"
#include "FramesSeriesUtils.h"
#include "FileSystemUtils.h"


// STD
#include <iostream>
#include <sstream>

// BOOST
#include <boost/filesystem.hpp>

// YAML
#include <yaml-cpp/yaml.h>

// VTK
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>



// FileSeries ------------------------------------------------------------------
void FileSeries::AddFile(std::string name, double time)
{
  Json::Value node;
  node["name"] = name;
  node["time"] = time;
  this->files.append(node);
}

void FileSeries::WriteToFile()
{
  if (this->filename.size() > 0)
  {
    Json::Value series;

    series["files"] = this->files;
    series["file-series-version"] = "1.0";
    std::ofstream fileSeriesStream(this->filename);
    fileSeriesStream << series;
    fileSeriesStream.close();
  }
}

//------------------------------------------------------------------------------
size_t FramesSeries::GetNumberOfClouds(std::string cloudFrameSeries)
{
  YAML::Node series = YAML::LoadFile(cloudFrameSeries);
  return series["files"].size();
}

void FramesSeries::ReadFromSeries(std::string fileSeries, size_t index, std::string& path, double& time)
{
  YAML::Node series = YAML::LoadFile(fileSeries);
  YAML::Node files = series["files"];

  // compute absolute file paths from the relative one present in the .series
  boost::filesystem::path dirname = boost::filesystem::path(fileSeries).parent_path();
  boost::filesystem::path basename = boost::filesystem::path(files[index]["name"].as<std::string>());
  path = (dirname / basename).string();

  time = files[index]["time"].as<double>();
}

std::pair<std::string, double> FramesSeries::GetClosestItemInSeries(std::string filename,
                                                      double time,
                                                      double maxTemporalDist)
{
  YAML::Node imageInfo = YAML::LoadFile(filename);
  double minTemporalDist = std::numeric_limits<double>::max();
  double closestImgTime = 0;
  std::string closestImgName;
  for (unsigned int imgIndex = 0; imgIndex < imageInfo["files"].size(); ++imgIndex)
  {
    double imgTime = imageInfo["files"][imgIndex]["time"].as<double>();
    if (std::abs(imgTime - time) < minTemporalDist)
    {
      minTemporalDist = std::abs(imgTime - time);
      closestImgTime = imgTime;
      closestImgName = RemoveExtension(imageInfo["files"][imgIndex]["name"].as<std::string>());
    }
  }

  if (minTemporalDist > maxTemporalDist) {
      std::cout << "image closest to lidar frame is too far in time. Are exports complete ? You could try running on less lidar frames" << std::endl;
      exit(EXIT_FAILURE);
  }

  return std::pair<std::string, double>(closestImgName, closestImgTime);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> FramesSeries::ReadCloudFrame(std::string pathToVTP)
{
  vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName(pathToVTP.c_str());
  reader->Update();
  vtkSmartPointer<vtkPolyData> cloud = reader->GetOutput();

  return cloud;
}

//------------------------------------------------------------------------------
void FramesSeries::GetWritePathFromSeries(std::string fileSeries, size_t index, std::string& path,  std::string& dirname)
{
  YAML::Node series = YAML::LoadFile(fileSeries);
  YAML::Node files = series["files"];

  // compute absolute file paths from the relative one present in the .series
  boost::filesystem::path basename = boost::filesystem::path(files[index]["name"].as<std::string>());
  path = (dirname / basename).string();
}


//------------------------------------------------------------------------------
int FramesSeries::WriteCloudFrame(vtkSmartPointer<vtkPolyData> cloud, std::string pathToVTP)
{
  vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetFileName(pathToVTP.c_str());
  writer->SetInputData(cloud);
  writer->Write();

  return EXIT_SUCCESS;
}


//------------------------------------------------------------------------------
std::pair<Eigen::Matrix3d, Eigen::Vector3d> GetRTFromTime(vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                                          double time)
{
  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  interpolator->InterpolateTransform(time, transform);
  vtkSmartPointer<vtkMatrix4x4> M0 = vtkSmartPointer<vtkMatrix4x4>::New();
  transform->GetMatrix(M0);
  Eigen::Matrix3d R0;
  Eigen::Vector3d T0;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      R0(i, j) = M0->Element[i][j];
    }
    T0(i) = M0->Element[i][3];
  }

  return std::pair<Eigen::Matrix3d, Eigen::Vector3d>(R0, T0);
}


//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> ReferenceFrameChange(vtkSmartPointer<vtkPolyData> cloud,
                                                  vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                                  double time)
{
  vtkSmartPointer<vtkPolyData> transformedCloud = vtkSmartPointer<vtkPolyData>::New();
  transformedCloud->DeepCopy(cloud);
  vtkSmartPointer<vtkDataArray> timeArray = transformedCloud->GetPointData()->GetArray("adjustedtime");

  // Get the target reference frame pose
  std::pair<Eigen::Matrix3d, Eigen::Vector3d> H0 = GetRTFromTime(interpolator, time);

  // transforms the points
  for (int pointIdx = 0; pointIdx < transformedCloud->GetNumberOfPoints(); ++pointIdx)
  {
    // Get the point and its time
    double pt[3];
    transformedCloud->GetPoint(pointIdx, pt);
    double ptTime = 1e-6 * static_cast<double>(timeArray->GetTuple1(pointIdx));
    Eigen::Vector3d X(pt[0], pt[1], pt[2]);

    // Get the transformed associated to the current time
    std::pair<Eigen::Matrix3d, Eigen::Vector3d> H1 = GetRTFromTime(interpolator, ptTime);

    // Reference frame changing
    Eigen::Vector3d Y = H0.first.transpose() * (H1.first * X + H1.second - H0.second);
    double newPt[3] = {Y(0), Y(1), Y(2)};
    transformedCloud->GetPoints()->SetPoint(pointIdx, newPt);
  }

  return transformedCloud;
}
