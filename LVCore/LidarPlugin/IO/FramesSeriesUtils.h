/*
  This module contains common tools for cloud/images frames series reading and time interpolation
  It concerns a sequence of data with the following format:
    data_folder
    |_ yyy.xx.series
    |_ frame1.xx
    |_ frame2.xx
    |_ ...

  yyy.xx.series is a json file with the filenames / time infos for the sequence in the following format:
    {
    "files": [
        {
        "name": "0000.jpg",
        "time": 1796.3160799999998
        },
        {
        "name": "0001.jpg",
        "time": 1796.3785599999999
        },
        ...
      ]
    }
*/

#ifndef FRAMESSERIESUTILS_H
#define FRAMESSERIESUTILS_H

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtk_jsoncpp.h>

#include "vtkEigenTools.h"

#include <string>

class FileSeries
{
  std::string filename = "";
  Json::Value files;

public:
  FileSeries() { };
  FileSeries(std::string filename): filename(filename) { };
  ~FileSeries() { };
  void AddFile(std::string name, double time);
  void WriteToFile();

};

// Get number of frames in a series
size_t GetNumberOfClouds(std::string cloudFrameSeries);

// Read file/time from series file value at given index
void ReadFromSeries(std::string fileSeries, size_t index, std::string& path, double& time);

// Remove extension from filename
std::string RemoveExtension(const std::string& filename);

// Return name (without extension) + time for closest frame in series described in file
std::pair<std::string, double> GetClosestItemInSeries(std::string filename,
                                                      double time,
                                                      double maxTemporalDist);

// Fill cloud from frame path
vtkSmartPointer<vtkPolyData> ReadCloudFrame(std::string pathToVTP);

// Write cloud frame to vtp
void GetWritePathFromSeries(std::string fileSeries, size_t index, std::string& path,  std::string& dirname);
int WriteCloudFrame(vtkSmartPointer<vtkPolyData> cloud, std::string pathToVTP);

// Cloud interpolation
std::pair<Eigen::Matrix3d, Eigen::Vector3d> GetRTFromTime(vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                                          double time);

vtkSmartPointer<vtkPolyData> ReferenceFrameChange(vtkSmartPointer<vtkPolyData> cloud,
                                                    vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                                    double time);


#endif // FRAMESSERIESUTILS_H
