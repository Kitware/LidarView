/*=========================================================================

  Program:   LidarView
  Module:    vtkImageColorToPointCloud.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkImageColorToPointCloud_h
#define vtkImageColorToPointCloud_h

#include <string>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersVisualizationModule.h"

class vtkInformation;
class vtkInformationVector;

/**
 * The vtkImageColorToPointCloud class projects color information from a
 * regular image grid onto a point cloud.
 *
 * This filter takes an image represented as a vtkDataSet and a point
 * cloud as vtkPolyData. When the structured grid forms a 2D regular grid
 * (typically lying in a plane, e.g., z = 0) the filter maps each point to the
 * grid indices using the grid's basis and transfers the corresponding grid
 * color using nearest-neighbor sampling.
 */
class LVFILTERSVISUALIZATION_EXPORT vtkImageColorToPointCloud : public vtkPolyDataAlgorithm
{
public:
  static vtkImageColorToPointCloud* New();
  vtkTypeMacro(vtkImageColorToPointCloud, vtkPolyDataAlgorithm);

  // Axis selection for image plane
  enum Axis
  {
    AXIS_X = 0, // X is constant, image lies in the YZ plane
    AXIS_Y = 1, // Y is constant, image lies in the XZ plane
    AXIS_Z = 2  // Z is constant, image lies in the XY plane
  };

  ///@{
  /**
   * Defines which world axis is normal to the image plane.
   *
   * The image lies in the plane obtained by dropping the selected world axis.
   * The remaining two axes define the in-plane pixel directions, and indices
   * are computed by projecting onto this plane using the grid basis.
   */
  vtkSetMacro(DropAxis, int);
  vtkGetMacro(DropAxis, int);
  ///@}

  ///@{
  /**
   * Name of the array on the input image dataset used for coloring.
   * If empty, the filter will try "RGB" and then the active scalars.
   */
  vtkSetStdStringFromCharMacro(ColorArrayName);
  vtkGetCharFromStdStringMacro(ColorArrayName);
  ///@}

protected:
  vtkImageColorToPointCloud();
  ~vtkImageColorToPointCloud() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkImageColorToPointCloud(const vtkImageColorToPointCloud&) = delete;
  void operator=(const vtkImageColorToPointCloud&) = delete;

  int DropAxis = AXIS_Z;
  std::string ColorArrayName;
};

#endif
