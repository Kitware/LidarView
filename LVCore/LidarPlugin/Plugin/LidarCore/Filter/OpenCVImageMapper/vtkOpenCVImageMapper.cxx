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


// LOCAL
#include "Filter/vtkOpenCVImageMapper.h"
#include "Common/vtkOpenCVConversions.h"
#include "vtkPipelineTools.h"

// VTK
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkHelper.h>
#include <vtkStreamingDemandDrivenPipeline.h>

// OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

// BOOST
#include <boost/filesystem.hpp>

//STD
#include <iostream>


constexpr unsigned int COORDS_IMAGE_INPUT_PORT = 0;
constexpr unsigned int TEXTURE_IMAGE_INPUT_PORT = 1;
constexpr unsigned int INPUT_PORTS_COUNT = 2;


vtkStandardNewMacro(vtkOpenCVImageMapper)

//-----------------------------------------------------------------------------
vtkOpenCVImageMapper::vtkOpenCVImageMapper()
{
  this->SetNumberOfInputPorts(INPUT_PORTS_COUNT);
}

//-----------------------------------------------------------------------------
int vtkOpenCVImageMapper::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == TEXTURE_IMAGE_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData" );
    return 1;
  }
  if (port == COORDS_IMAGE_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData" );
    return 1;
  }
  return 0;
}


//-----------------------------------------------------------------------------
int vtkOpenCVImageMapper::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{
  vtkInformation* inTextureInfo = inputVector[TEXTURE_IMAGE_INPUT_PORT]->GetInformationObject(0);
  if (inTextureInfo != nullptr)
  {
    // RequestUpdateExtent is called in the opposite direction (going backward in the pipeline), so we get the requestedTimestamp by looking at the output
    double requestedTimestamp = outputVector->GetInformationObject(0)->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    std::vector<double> textureImageTimesteps = getTimeSteps(inTextureInfo);
    int bestImageTimeId = closestElementInOrderedVector(textureImageTimesteps, requestedTimestamp);
    double bestImageTime = textureImageTimesteps[bestImageTimeId];

    vtkDebugMacro(<< "vtkOpenCVImageMapper::RequestUpdateExtent() with time: "
                  << requestedTimestamp
                  << " closest image time: " << bestImageTime);

    // Position the chosen image in input of the filter, and keep track of its (pipeline) time
    inTextureInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), bestImageTime);
  }
  return 1;
}


//-----------------------------------------------------------------------------
int vtkOpenCVImageMapper::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkImageData* inCoordsImg = vtkImageData::GetData(inputVector[COORDS_IMAGE_INPUT_PORT], 0);
  vtkImageData* inTextureImg = vtkImageData::GetData(inputVector[TEXTURE_IMAGE_INPUT_PORT], 0);
  vtkImageData* outImg = vtkImageData::GetData(outputVector, 0);

  outImg->DeepCopy(inCoordsImg);

  // Convert texture image to openCV format
  cv::Mat inOpenCVImg = VtkImageToCvImage(inTextureImg, false);

  // Get opencv maps from texture coordinates
  auto tcoordsArray = outImg->GetPointData()->GetArray("TextureCoordinates");
  if (tcoordsArray == nullptr)
  {
    vtkErrorMacro("Coords input daoes not have a 'TextureCoordinates' point data");
  }
  auto floatTCoords = vtkSmartPointer<vtkFloatArray>::New();
  floatTCoords->DeepCopy(tcoordsArray);  // Deep copy to convert to float data
  float* tcoordsData = floatTCoords->GetPointer(0);

  int inDims[3];
  inTextureImg->GetDimensions(inDims);
  int outDims[3];
  inCoordsImg->GetDimensions(outDims);

  cv::Mat outOpenCVImg(outDims[1], outDims[0], CV_8UC3);

  cv::Mat maps(outDims[1], outDims[0], CV_32FC2, tcoordsData);
  cv::Mat separateMaps[2];
  cv::split(maps, separateMaps);
  separateMaps[0] *= inDims[0];
  separateMaps[1] *= inDims[1];
  cv::remap(inOpenCVImg, outOpenCVImg, separateMaps[0], separateMaps[1], cv::INTER_LINEAR,
            cv::BORDER_CONSTANT, cv::Scalar(0));

  // Check for valid data (from vtkValidPointMask + potentially Invalid texture coords)

  // vtkValidPointMask is a signed char array
  vtkCharArray* validArray = vtkCharArray::SafeDownCast(
          outImg->GetPointData()->GetArray("vtkValidPointMask"));
  char* validData = validArray->GetPointer(0);

  // validCVMat points to the data in validArray, hence its data type is signed chars as well
  cv::Mat validCVMat(outDims[1], outDims[0], CV_8SC1, validData);

  // Make points with invalid texture coordinates invalid
  if (this->RemovePointsWithInvalidTexture)
  {
    for (vtkIdType pointIndex = 0; pointIndex < outImg->GetNumberOfPoints(); ++pointIndex)
    {
      double* tcoords = floatTCoords->GetTuple2(pointIndex);
      if ((tcoords[0] < 0) || (tcoords[0] >= 1) || (tcoords[1] < 0) || (tcoords[1] >= 1))
      {
        validArray->SetTuple1(pointIndex, 0);
      }
    }
  }

  // Save image to file
  if (this->SaveToFile)
  {
    if (this->OutFolderPath.empty())
    {
      vtkWarningMacro("No folder path provided, skip saving output images.");
    }
    else
    {
      std::vector<cv::Mat> bgraChannels(3);
      cv::split(outOpenCVImg, bgraChannels);
      cv::Mat alphaUnsigned;
      // Opencv requires an unsigned chars array with values in 0-255
      // It is converted from the validData array of signed chars in 0-1 and scaled
      validCVMat.convertTo(alphaUnsigned, CV_8UC1, 255);
      bgraChannels.push_back(alphaUnsigned);
      cv::Mat bgraImg;
      cv::merge(bgraChannels, bgraImg);
      cv::flip(bgraImg, bgraImg, 0);

      auto dirname = boost::filesystem::path(this->OutFolderPath);
      std::stringstream ss;
      ss << std::setw(6) << std::setfill('0') << this->FrameCount;
      std::string s = ss.str();
      auto basename = boost::filesystem::path(s + ".png");
      cv::imwrite((dirname / basename).string(), bgraImg);
      this->FrameCount += 1;
    }
  }

  // Copy pixel values to output image
  cv::cvtColor(outOpenCVImg, outOpenCVImg, cv::COLOR_BGR2RGB);
  auto rgbArray = createArray<vtkUnsignedCharArray>("RGB", 3, inCoordsImg->GetNumberOfPoints());
  std::memcpy(rgbArray->GetPointer(0), outOpenCVImg.data, outDims[0] * outDims[1] * 3);
  outImg->GetPointData()->AddArray(rgbArray);

  return 1;
}
