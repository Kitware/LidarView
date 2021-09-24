//=========================================================================
//
// Copyright 2020 Kitware, Inc.
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

#include "LidarCoreModule.h"


class LIDARCORE_EXPORT FileSeries
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

namespace FramesSeries
{
  // Get number of frames in a series
  size_t LIDARCORE_EXPORT GetNumberOfClouds(std::string cloudFrameSeries);

  // Read file/time from series file value at given index
  void LIDARCORE_EXPORT ReadFromSeries(std::string fileSeries, size_t index, std::string& path, double& time);

  // Return name (without extension) + time for closest frame in series described in file
  std::pair<std::string, double> LIDARCORE_EXPORT GetClosestItemInSeries(std::string filename,
                                                        double time,
                                                        double maxTemporalDist);

  // Fill cloud from frame path
  vtkSmartPointer<vtkPolyData> LIDARCORE_EXPORT ReadCloudFrame(std::string pathToVTP);

  // Write cloud frame to vtp
  void LIDARCORE_EXPORT GetWritePathFromSeries(std::string fileSeries, size_t index, std::string& path,  std::string& dirname);
  int LIDARCORE_EXPORT WriteCloudFrame(vtkSmartPointer<vtkPolyData> cloud, std::string pathToVTP);

}

// Cloud interpolation
std::pair<Eigen::Matrix3d, Eigen::Vector3d> LIDARCORE_EXPORT GetRTFromTime(vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                                          double time);

vtkSmartPointer<vtkPolyData> LIDARCORE_EXPORT ReferenceFrameChange(vtkSmartPointer<vtkPolyData> cloud,
                                                    vtkSmartPointer<vtkCustomTransformInterpolator> interpolator,
                                                    double time);


#endif // FRAMESSERIESUTILS_H
