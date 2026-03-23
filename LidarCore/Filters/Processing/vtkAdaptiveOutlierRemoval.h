/*=========================================================================

  Program: LidarView
  Module:  vtkAdaptiveOutlierRemoval.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_ADAPTIVE_OUTLIER_REMOVAL_H
#define VTK_ADAPTIVE_OUTLIER_REMOVAL_H

// VTK
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include "lvFiltersProcessingModule.h"

/**
 * @brief This filter identifies outliers in a point cloud based on the average distance of
 neighboring points. The threshold can be defined adaptivly with respect to the depth of points.
 */

class LVFILTERSPROCESSING_EXPORT vtkAdaptiveOutlierRemoval : public vtkPolyDataAlgorithm
{
public:
  static vtkAdaptiveOutlierRemoval* New();
  vtkTypeMacro(vtkAdaptiveOutlierRemoval, vtkPolyDataAlgorithm)

  // Set number of neighbor points to use
  vtkSetClampMacro(NbNeighbors, int, 1, VTK_INT_MAX);
  vtkGetMacro(NbNeighbors, int);

  // Set average distance threshold
  vtkSetMacro(AveDistThreshold, double);
  vtkGetMacro(AveDistThreshold, double);

  // Enable/disable adaptive outlier removal
  vtkSetMacro(EnableAdaptiveRemoval, bool);
  vtkGetMacro(EnableAdaptiveRemoval, bool);

  // Set factor of the linear function to define the threshold
  vtkSetMacro(Factor, double);
  vtkGetMacro(Factor, double);

  // Set/get search radius factor for adaptive clustering
  vtkSetMacro(Scalar1Weight, float);
  vtkGetMacro(Scalar1Weight, float);

  // Set/get search radius factor for adaptive clustering
  vtkSetMacro(Scalar2Weight, float);
  vtkGetMacro(Scalar2Weight, float);

protected:
  vtkAdaptiveOutlierRemoval();
  ~vtkAdaptiveOutlierRemoval() = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAdaptiveOutlierRemoval(const vtkAdaptiveOutlierRemoval&) = delete;
  void operator=(const vtkAdaptiveOutlierRemoval&) = delete;

  void RemoveOutlier(vtkSmartPointer<vtkPolyData> inputPointcloud,
    vtkSmartPointer<vtkPolyData> outputPointcloud);

  // Depth array name
  std::string DepthArrayName;
  // Scalar array name
  // Extra dimensions to compute distance
  std::string Scalar1ArrayName;
  std::string Scalar2ArrayName;
  // Scalar weight
  float Scalar1Weight = 1.0;
  float Scalar2Weight = 1.0;
  // Number of neighbor points to compute the average distance for one point
  int NbNeighbors = 10;
  // The threshold to remove one point
  double AveDistThreshold = 0.1;
  // Enable adaptive outlier removal
  bool EnableAdaptiveRemoval = true;
  // The factor of the linear function of depth to define the threshold
  // The default linear function is based on the arc length of an angular resolution :
  // Threshold = angular resolution * depth
  // The default value is computed based on an angular resolution 0.2° (0.2 * pi / 180)
  double Factor = 0.0035;
};

#endif // VTK_ADAPTIVE_OUTLIER_REMOVAL_H
