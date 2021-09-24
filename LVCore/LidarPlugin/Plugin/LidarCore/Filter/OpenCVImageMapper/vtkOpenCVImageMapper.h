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


#ifndef VTK_OPENCV_IMAGE_MAPPER_H
#define VTK_OPENCV_IMAGE_MAPPER_H

// VTK
#include <vtkImageData.h>
#include <vtkImageAlgorithm.h>
#include <vtkPolyDataAlgorithm.h>

// OPENCV
#include <opencv2/core.hpp>

#include "LidarCoreModule.h"

/**
 * @brief vtkOpenCVImageMapper Takes an image with texture coordinates and an
 * image to map (texture image) and returns the first image with additionnal
 * remapped texture pixel values (in a 'RGB' point data array).
 * The mapping image (vtkImage) must contain TextureCoordinates PointData
 * It can additionnally save the output image as png files with alpha = 0
 * for invalid image points (using vtkValidPointMask).
 */

class LIDARCORE_EXPORT vtkOpenCVImageMapper : public vtkImageAlgorithm
{
public:
  static vtkOpenCVImageMapper *New();
  vtkTypeMacro(vtkOpenCVImageMapper, vtkImageAlgorithm)
  vtkSetMacro(SaveToFile, bool);
  vtkSetMacro(OutFolderPath, std::string);
  vtkSetMacro(RemovePointsWithInvalidTexture, bool);

protected:
  vtkOpenCVImageMapper();
  ~vtkOpenCVImageMapper() = default;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestUpdateExtent(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  vtkOpenCVImageMapper(const vtkOpenCVImageMapper&) = delete;
  void operator=(const vtkOpenCVImageMapper&) = delete;

  // If true, output images are saved to RGBA png files. The alpha value
  // corresponds to the vtkValidPointMask values (ie alpha=1 if a given pixel
  // corresponds to a value contained in the texture image)
  bool SaveToFile = false;
  // out filenames will be formatted like <OutFolderPath>/000012.png
  std::string OutFolderPath = "";

  // Counter of generated views used for output saving (TODO: use frame index)
  unsigned int FrameCount = 0;

  // If true, points with texture coordinates that are not in [0, 1[ are marked
  // as invalid in the output.
  bool RemovePointsWithInvalidTexture = true;
};

#endif // VTK_OPENCV_IMAGE_MAPPER_H
