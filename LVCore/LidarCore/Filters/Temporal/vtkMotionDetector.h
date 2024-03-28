// Copyright 2018 Kitware SAS
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
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMotionDetector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_MOTION_DETECTOR_H
#define VTK_MOTION_DETECTOR_H

// VTK
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include "lvFiltersTemporalModule.h"

#include <memory> // for std::unique_ptr

/**
 * @brief The MotionDetector class constructs a spherical map of depth along the
 * vertical angle and the azimuth angle. A gaussian mixture model is built
 * at each pixel and it is updated when a new point arrived. The probability
 * that a point is belong to background can be evaluated by GMM.
 * Input: Lidar frame
 * Output1: Lidar frame with motion probability
 * Output2: Pointcloud of motion objects labeled with cluster id
 */
class LVFILTERSTEMPORAL_EXPORT vtkMotionDetector : public vtkPolyDataAlgorithm
{
public:
  static vtkMotionDetector* New();
  vtkTypeMacro(vtkMotionDetector, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  // constructor / destructor
  vtkMotionDetector();
  ~vtkMotionDetector();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  // copy operators
  vtkMotionDetector(const vtkMotionDetector&);
  void operator=(const vtkMotionDetector&);

  /**
   * Internals parameters and functions of motion detector
   * Gaussian
   * Gaussian mixture
   * Spherical depth map
   */
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif // VTK_MOTION_DETECTOR_H
