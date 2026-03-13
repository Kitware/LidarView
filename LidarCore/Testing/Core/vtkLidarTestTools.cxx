/*=========================================================================

  Program: LidarView
  Module:  vtkTestUtilities.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarTestTools.h"
#include "TestHelpers.h"
#include "vtkLidarPacketInterpreter.h"
#include "vtkLidarReader.h"
#include "vtkLidarStream.h"

#include <vtkAbstractArray.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransform.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <numeric>

namespace
{
constexpr unsigned int ARRAYS_NB = 4;
constexpr const char* MANDATORY_ARRAYS[ARRAYS_NB] = { "intensity",
  "timestamp",
  "laser_id",
  "distance_m" };
constexpr double TOLERANCE_FRAME_DURATION = 6. * 1e3; // 6ms

//----------------------------------------------------------------------------
int ParseReferenceFile(const std::string& referenceDataFileName,
  std::string& outBaselinePath,
  double& outRefTime)
{
  vtkNew<vtkPVXMLParser> parser;
  parser->SetFileName(referenceDataFileName.c_str());
  if (parser->Parse() == 0 || parser->GetRootElement() == nullptr)
  {
    std::cerr << "Invalid XML in file: " << referenceDataFileName << "." << std::endl;
    return EXIT_FAILURE;
  }
  vtkPVXMLElement* root = parser->GetRootElement();

  const char* baselineFile = root->GetAttribute("baselineFile");
  if (!baselineFile)
  {
    std::cerr << "No baselineFile element was found." << std::endl;
    return EXIT_FAILURE;
  }
  outBaselinePath = std::filesystem::path(referenceDataFileName).parent_path().generic_string();
  outBaselinePath += "/" + std::string(baselineFile);

  const char* timeRefAttribute = root->GetAttribute("referenceNetworkTimeToDataTime");
  if (!timeRefAttribute)
  {
    std::cerr << "No referenceNetworkTimeToDataTime element was found." << std::endl;
    return EXIT_FAILURE;
  }
  outRefTime = std::stod(timeRefAttribute);
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int GetTimestampRange(vtkPolyData* data, double* range)
{
  vtkDoubleArray* array =
    vtkDoubleArray::SafeDownCast(data->GetPointData()->GetAbstractArray("timestamp"));
  if (!array)
  {
    std::cerr << "Error: Could not convert timestamp array to double!" << std::endl;
    return EXIT_FAILURE;
  }
  array->GetRange(range);
  return EXIT_SUCCESS;
}
}

//----------------------------------------------------------------------------
int vtkLidarTestTools::TestLidarReaderWithBaseline(vtkLidarReader* reader,
  const std::string& referenceDataFileName)
{
  std::string baselineFilePath;
  double outRefTime;

  if (::ParseReferenceFile(referenceDataFileName, baselineFilePath, outRefTime))
  {
    return EXIT_FAILURE;
  }

  return testLidarReader(reader, outRefTime, baselineFilePath);
}

//----------------------------------------------------------------------------
int vtkLidarTestTools::TestLidarStreamWithBaseline(vtkLidarStream* stream,
  const std::string& pcapFileName,
  const std::string& referenceDataFileName,
  bool shouldPreSend)
{
  // Auto stop stream
  stream->Update();
  stream->Stop();

  std::string baselineFilePath;
  double outRefTime;

  if (::ParseReferenceFile(referenceDataFileName, baselineFilePath, outRefTime))
  {
    return EXIT_FAILURE;
  }

  return testLidarStream(stream, pcapFileName, baselineFilePath, shouldPreSend);
}

//----------------------------------------------------------------------------
int vtkLidarTestTools::TestPacketInterpreterTimeFrames(vtkLidarReader* reader, int type)
{
  reader->SetDisplayTimeType(type);
  reader->Update();

  vtkInformation* info = reader->GetOutputInformation(0);
  unsigned int frameNb = 0;
  if (info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    frameNb = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  if (frameNb < 2)
  {
    std::cerr << "Error: Must have at least 2 frames." << std::endl;
    return EXIT_FAILURE;
  }
  unsigned int midFrameIdx = frameNb / 2;
  double frameDuration_s = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), midFrameIdx) -
    info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), midFrameIdx - 1);

  // Test that duration between two frames is between 5s and 5ms.
  if (frameDuration_s > 5. || frameDuration_s < 0.005)
  {
    std::cerr << "Error: The frames should be in seconds. The duration between two frames is not "
              << "between 5s and 5ms. Got: " << frameDuration_s << "s" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkPolyData> baseline;
  baseline->DeepCopy(GetCurrentFrame(reader, 0));
  vtkPointData* pData = baseline->GetPointData();

  // Test that timestamp array is different for all points
  vtkDoubleArray* array = vtkDoubleArray::SafeDownCast(pData->GetAbstractArray("timestamp"));
  if (!array)
  {
    std::cerr << "Error: Could not convert timestamp array to double!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkNew<vtkDoubleArray> timestamps;
  timestamps->DeepCopy(array);

  std::sort(timestamps->Begin(), timestamps->End());
  auto found = std::adjacent_find(timestamps->Begin(), timestamps->End());
  if (found != timestamps->End())
  {
    std::cerr << "Error: Found a duplicated timestamp value!" << std::endl;
    return EXIT_FAILURE;
  }

  double range[2];
  timestamps->GetRange(range);
  double duration = std::abs(range[1] - range[0]);
  // Check point timestamp duration is coherent with frame duration (in microseconds)
  if (duration < frameDuration_s * 1e6 - ::TOLERANCE_FRAME_DURATION ||
    duration > frameDuration_s * 1e6 + ::TOLERANCE_FRAME_DURATION)
  {
    std::cerr << "Error: Timestamps should be in microseconds! Got range: " << duration
              << " and frame duration " << frameDuration_s * 1e6 << "μs (± 6ms)" << std::endl;
    return EXIT_FAILURE;
  }

  // Check if the last point timestamp of frame n is earlier
  // than the first point timestamp of frame n + 1.
  for (unsigned int idx = 0; idx < frameNb - 1; idx++)
  {
    double firstRange[2];
    double secondRange[2];
    if (::GetTimestampRange(GetCurrentFrame(reader, idx), firstRange) ||
      ::GetTimestampRange(GetCurrentFrame(reader, idx + 1), secondRange))
    {
      return EXIT_FAILURE;
    }

    if (firstRange[1] > secondRange[0])
    {
      std::cerr << std::setprecision(12);
      std::cerr << "Error: The last point timestamp of frame " << idx << " (" << firstRange[1]
                << "μs) is greater than the first point timestamp of frame " << idx + 1 << " ("
                << secondRange[0] << ")" << std::endl;
      std::cerr << "Note: it might be standard for some future interpreters, in this case please "
                   "change the test."
                << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int vtkLidarTestTools::TestPacketInterpreter(vtkLidarReader* reader)
{
  // Test for time issues
  if (TestPacketInterpreterTimeFrames(reader, vtkLidarReader::USE_LIDAR_TIME))
  {
    return EXIT_FAILURE;
  }
  if (TestPacketInterpreterTimeFrames(reader, vtkLidarReader::USE_NETWORK_TIME))
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkPolyData> baseline;
  baseline->DeepCopy(GetCurrentFrame(reader, 0));

  // Test for "mandatory" array names used in LV filters & in SLAM
  vtkPointData* pData = baseline->GetPointData();
  for (unsigned int idx = 0; idx < ::ARRAYS_NB; idx++)
  {
    vtkAbstractArray* array = pData->GetAbstractArray(::MANDATORY_ARRAYS[idx]);
    if (!array)
    {
      std::cerr << "Error: Missing mandatory array: " << ::MANDATORY_ARRAYS[idx] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test that transform change coordinates for all points
  vtkNew<vtkTransform> transform;
  transform->Translate(50, 23, 12);
  reader->GetLidarInterpreter()->SetSensorTransform(transform);
  reader->Update();

  vtkPolyData* result = GetCurrentFrame(reader, 0);

  if (baseline->GetNumberOfPoints() != result->GetNumberOfPoints())
  {
    std::cerr << "ERROR: Baseline and result doesn't have the same number of points." << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int idx = 0; idx < baseline->GetNumberOfPoints(); idx++)
  {
    double baselinePoint[3];
    double resultPoint[3];
    baseline->GetPoint(idx, baselinePoint);
    result->GetPoint(idx, resultPoint);

    transform->TransformPoint(baselinePoint, baselinePoint);

    if (!compare(resultPoint, baselinePoint, 1e-5))
    {
      std::cerr << "ERROR: Points at idx: " << idx << " are not equal. (" << toString(baselinePoint)
                << " != " << toString(resultPoint) << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
