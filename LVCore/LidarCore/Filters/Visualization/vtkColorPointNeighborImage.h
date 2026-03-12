/*=========================================================================

  Program:   LidarView
  Module:    vtkColorPointNeighborImage.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkColorPointNeighborImage_h
#define vtkColorPointNeighborImage_h

#include <string>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersVisualizationModule.h"

class vtkInformation;
class vtkInformationVector;

/**
 * The vtkColorPointNeighborImage class projects color information from an
 * image onto a point cloud using nearest-neighbor lookup.
 *
 * This filter takes an image represented as a vtkDataSet and a point
 * cloud as vtkPolyData. For each point in the cloud, it finds the closest point
 * in the image grid and transfers its color (RGB or scalar-based) to the point
 * cloud.
 *
 * Compared with classic camera-based colorization (camera image + lidar points +
 * extrinsic/intrinsic calibration), this filter does not perform camera
 * projection. It assumes the input image dataset and input point cloud are
 * already spatially aligned in the same coordinate frame.
 */
class LVFILTERSVISUALIZATION_EXPORT vtkColorPointNeighborImage : public vtkPolyDataAlgorithm
{
public:
  static vtkColorPointNeighborImage* New();
  vtkTypeMacro(vtkColorPointNeighborImage, vtkPolyDataAlgorithm);

  /**
   * Interpretation of input image array.
   * 0: Color image, 1: Grayscale, 2: Mask.
   */
  enum ImageInterpretation
  {
    ColorImage = 0,
    Grayscale = 1,
    Mask = 2
  };

  /**
   * Image type used to interpret input pixel values.
   */
  vtkSetClampMacro(ImageType, int, 0, 2);
  vtkGetMacro(ImageType, int);

  /**
   * Name of the point-data array on the input image dataset used for colors.
   */
  vtkSetStdStringFromCharMacro(ColorArrayName);
  vtkGetCharFromStdStringMacro(ColorArrayName);

  /**
   * Name of the output point-data color array created on the point cloud.
   * Defaults to "RGB".
   */
  vtkSetStdStringFromCharMacro(OutputArrayName);
  vtkGetCharFromStdStringMacro(OutputArrayName);

protected:
  vtkColorPointNeighborImage();
  ~vtkColorPointNeighborImage() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkColorPointNeighborImage(const vtkColorPointNeighborImage&) = delete;
  void operator=(const vtkColorPointNeighborImage&) = delete;

  std::string ColorArrayName;
  std::string OutputArrayName;
  int ImageType;
};

#endif
