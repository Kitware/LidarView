// Copyright 2013 Velodyne Acoustics, Inc.
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

#include "TestHelpers.h"

#include "vtkLidarReader.h"
#include "vtkLidarStream.h"

#include <vtkCommand.h>
#include <vtkExecutive.h>
#include <vtkInformation.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkTimerLog.h>

#include <vvPacketSender.h>

#include <sstream>

#include <boost/thread/thread.hpp>

// Helper functions
//-----------------------------------------------------------------------------
bool compare(const double* const a, const double* const b, const size_t N, double epsilon)
{
  for (size_t i = 0; i < N; ++i)
  {
    if (!vtkMathUtilities::FuzzyCompare(a[i], b[i], epsilon))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
std::string toString(const double* const d, const size_t N)
{
  std::ostringstream strs;
  strs << "(";

  strs << d[0];
  for (size_t i = 1; i < N; ++i)
  {
    strs << ", " << d[i];
  }

  strs << ")";

  return strs.str();
}

//-----------------------------------------------------------------------------
int GetNumberOfTimesteps(vtkLidarStream* HDLSource)
{
  HDLSource->UpdateInformation();

  vtkInformation* outInfo = HDLSource->GetExecutive()->GetOutputInformation(0);

  return outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
}

//-----------------------------------------------------------------------------
vtkPolyData* GetCurrentFrame(vtkLidarReader* HDLreader, int index)
{
  HDLreader->Open();
  vtkPolyData* currentFrame = HDLreader->GetFrame(index);
  HDLreader->Close();

  return currentFrame;
}

//-----------------------------------------------------------------------------
vtkPolyData* GetCurrentFrame(vtkLidarStream* HDLsource, int index)
{
  HDLsource->UpdateInformation();

  vtkInformation* outInfo = HDLsource->GetExecutive()->GetOutputInformation(0);

  double* timeSteps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  double updateTime = timeSteps[index];
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);

  HDLsource->Update();

  HDLsource->GetOutput()->Register(NULL);

  vtkPolyData* currentFrame = HDLsource->GetOutput();

  return currentFrame;
}

//-----------------------------------------------------------------------------
std::vector<std::string> GenerateFileList(const std::string& metaFileName)
{
  int lastSeparator = metaFileName.find_last_of("/\\");
  std::string dir = metaFileName.substr(0, lastSeparator);

  std::ifstream metaFile(metaFileName.c_str());
  std::string line;
  std::vector<std::string> filenameList;

  if (metaFile.good())
  {
    while (metaFile >> line)
    {
      if (line.size() > 0)
      {
        std::string fullPath = dir + "/" + line;
        filenameList.push_back(fullPath);
      }
    }
  }

  return filenameList;
}

//-----------------------------------------------------------------------------
vtkPolyData* GetCurrentReference(const std::vector<std::string>& referenceFilesList, int index)
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(referenceFilesList[index].c_str());
  reader->Update();

  reader->GetOutput()->Register(NULL);

  return reader->GetOutput();
}

// Test functions
//-----------------------------------------------------------------------------
int TestFrameCount(unsigned int frameCount, unsigned int referenceCount)
{
  std::cout << "Frame count : \t";
  if (frameCount != referenceCount)
  {
    std::cerr << "failed : expected " << referenceCount << ", got " << frameCount
              << std::endl;

    return 1;
  }
  std::cout << "passed" << std::endl;
  return 0;
}

//-----------------------------------------------------------------------------
int TestPointCount(vtkPolyData* currentFrame, vtkPolyData* currentReference)
{
  std::cout << "Point Count : \t";
  // Compare the point counts
  vtkIdType currentFrameNbPoints = currentFrame->GetNumberOfPoints();
  vtkIdType currentReferenceNbPoints = currentReference->GetNumberOfPoints();

  if (currentFrameNbPoints != currentReferenceNbPoints)
  {
    std::cerr << "failed : expected " << currentReferenceNbPoints << ", got "
              << currentFrameNbPoints << std::endl;

    return 1;
  }
  std::cout << "passed" << std::endl;
  return 0;
}

//-----------------------------------------------------------------------------
int TestPointDataStructure(vtkPolyData* currentFrame, vtkPolyData* currentReference)
{
  std::cout << "Data Structure : \t";
  int retVal = 0;

  // Get the current frame point data
  vtkPointData* currentFramePointData = currentFrame->GetPointData();

  // Get the reference point data corresponding to the current frame
  vtkPointData* currentReferencePointData = currentReference->GetPointData();

  // Compare the number of arrays
  int currentFrameNbPointDataArrays = currentFramePointData->GetNumberOfArrays();
  int currentReferenceNbPointDataArrays = currentReferencePointData->GetNumberOfArrays();

  if (currentFrameNbPointDataArrays != currentReferenceNbPointDataArrays)
  {
    std::cerr << "failed :Wrong point data array count. Expected " << currentReferenceNbPointDataArrays
              << ", got " << currentFrameNbPointDataArrays << std::endl;

    return 1;
  }

  // For each array, Checks the structure i.e. the name, number of components &
  // number of tuples matches
  for (int idArray = 0; idArray < currentFrameNbPointDataArrays; ++idArray)
  {
    vtkDataArray* currentFrameArray = currentFramePointData->GetArray(idArray);
    vtkDataArray* currentReferenceArray = currentReferencePointData->GetArray(idArray);

    std::string currentFrameArrayName = currentFrameArray->GetName();
    std::string currentReferenceArrayName = currentReferenceArray->GetName();

    if (currentFrameArrayName != currentReferenceArrayName)
    {
      std::cerr << "failed : Wrong array name for frame. Expected " << currentReferenceArrayName << ", got"
                << currentFrameArrayName << std::endl;

      return 1;
    }

    int currentFrameNbComponents = currentFrameArray->GetNumberOfComponents();
    int currentReferenceNbComponents = currentReferenceArray->GetNumberOfComponents();

    if (currentFrameNbComponents != currentReferenceNbComponents)
    {
      std::cerr << " failed : Wrong number of components for array " << currentReferenceArrayName
                << ". Expected " << currentReferenceNbComponents << ", got "
                << currentFrameNbComponents << std::endl;

      return 1;
    }

    vtkIdType currentFrameNbTuples = currentFrameArray->GetNumberOfTuples();
    vtkIdType currentReferenceNbTuples = currentReferenceArray->GetNumberOfTuples();

    if (currentFrameNbTuples != currentReferenceNbTuples)
    {
      std::cerr << "failed : Wrong number of components for array " << currentReferenceArrayName
                << " at frame. Expected " << currentReferenceNbTuples << ", got "
                << currentFrameNbTuples << std::endl;

      return 1;
    }
  }
  std::cout << "passed" << std::endl;
  return retVal;
}

//-----------------------------------------------------------------------------
int TestPointDataValues(vtkPolyData* currentFrame, vtkPolyData* currentReference)
{
  std::cout << "Point Data Value : \t";
  int retVal = 0;

  // Get the current frame point data
  vtkPointData* currentFramePointData = currentFrame->GetPointData();

  // Get the reference point data corresponding to the current frame
  vtkPointData* currentReferencePointData = currentReference->GetPointData();

  // For each array, Checks the values
  for (int idArray = 0; idArray < currentReferencePointData->GetNumberOfArrays(); ++idArray)
  {
    const char * arrayName = currentFramePointData->GetArrayName(idArray);
    vtkDataArray* currentFrameArray = currentFramePointData->GetArray(arrayName);
    vtkDataArray* currentReferenceArray = currentReferencePointData->GetArray(arrayName);

    for (int idTuple = 0; idTuple < currentReferenceArray->GetNumberOfTuples(); ++idTuple)
    {
      int nbComp = currentReferenceArray->GetNumberOfComponents();

      double* frameTuple = currentFrameArray->GetTuple(idTuple);
      double* referenceTuple = currentReferenceArray->GetTuple(idTuple);

      if (!compare(frameTuple, referenceTuple, nbComp, 1e-12))
      {
        if ((long)(*referenceTuple - *frameTuple) % 3600 * 1e6 == 0)
          continue;
        std::cerr << "failed : Tuples " << idTuple << " doesn't match for array " << idArray << " ("
                  << currentReferenceArray->GetName() << "). Expected "
                  << toString(referenceTuple, nbComp) << ", got " << toString(frameTuple, nbComp)
                  << std::endl;

        return 1;
      }
    }
  }
  std::cout << "passed" << std::endl;
  return retVal;
}

//-----------------------------------------------------------------------------
int TestPointPositions(vtkPolyData* currentFrame, vtkPolyData* currentReference)
{
  std::cout << "Point Position : \t";
  int retVal = 0;

  // Get the current frame points
  vtkPoints* currentFramePoints = currentFrame->GetPoints();

  // Get the reference points corresponding to the current frame
  vtkPoints* currentReferencePoints = currentReference->GetPoints();

  // Compare each points
  double framePoint[3];
  double referencePoint[3];

  for (int currentPointId = 0; currentPointId < currentFramePoints->GetNumberOfPoints();
       ++currentPointId)
  {
    currentFramePoints->GetPoint(currentPointId, framePoint);
    currentReferencePoints->GetPoint(currentPointId, referencePoint);

    if (!compare(framePoint, referencePoint, 1e-12))
    {
      std::cerr << "failed : Wrong point coordinates at point " << currentPointId << ". Expected ("
                << toString(referencePoint) << ", got " << toString(framePoint) << std::endl;

      return 1;
    }
  }
  std::cout << "passed" << std::endl;
  return retVal;
}

//-----------------------------------------------------------------------------
int TestRPMValues(vtkPolyData* currentFrame, vtkPolyData* currentReference, double tol)
{
  auto currentFrameRPMArray = currentFrame->GetFieldData()->GetArray("RotationPerMinute");
  auto currentReferenceRPMArray = currentReference->GetFieldData()->GetArray("RotationPerMinute");

  if (currentFrameRPMArray == nullptr && currentReferenceRPMArray == nullptr)
  {
    return 0;
  }

  std::cout << "RPM Value : \t";
  // Get the current frame RPM
  double currentFrameRPM =
    currentFrame->GetFieldData()->GetArray("RotationPerMinute")->GetTuple1(0);

  // Get the reference point data corresponding to the current frame
  double currentReferenceRPM =
    currentReference->GetFieldData()->GetArray("RotationPerMinute")->GetTuple1(0);

  if (!vtkMathUtilities::FuzzyCompare(currentFrameRPM, currentReferenceRPM, tol))
  {
    std::cerr << "failed : Wrong RPM value. Expected " << currentReferenceRPM << ", got " << currentFrameRPM
              << std::endl;

    return 1;
  }
  std::cout << "passed" << std::endl;
  return 0;
}

//-----------------------------------------------------------------------------
int TestNetworkTimeToLidarTime(vtkLidarReader* HDLReader,
                               double referenceNetworkTimeToLidarTime)
{
  std::cout << "NetworkTimeToLidarTime : \t";
  double currentNetworkTimeToLidarTime = HDLReader->GetNetworkTimeToDataTime();

  // We do not want to be too strict because this timeshift is currently
  // computed using the timestamp of the first data packet inside the
  // network packet (not of the first used data packet) so this timeshift
  // can be improved.
  if (!vtkMathUtilities::FuzzyCompare(currentNetworkTimeToLidarTime,
                                      referenceNetworkTimeToLidarTime,
                                      1.0))
  {
    std::cerr << "failed : Wrong NetworkTimeToLidarTime value. Expected "
	      << referenceNetworkTimeToLidarTime
	      << ", got " << std::fixed << std::setprecision(17) << currentNetworkTimeToLidarTime
              << std::endl;

    return 1;
  }

  std::cout << "passed" << std::endl;
  return 0;
}

//-----------------------------------------------------------------------------
int testLidarReader(vtkLidarReader *reader,
                    double referenceNetworkTimeToDataTime,
                    const std::string& referenceFileName)
{
  // get VTP file name from the reference file
  std::vector<std::string> referenceFilesList;
  referenceFilesList = GenerateFileList(referenceFileName);

  // update the reader to have access to the frame
  reader->Update();

  // Check if we can read the PCAP file
  if (reader->GetNumberOfFrames() == 0)
  {
      std::cout << "ERROR, the reader ouput 0 frame!!" << std::endl
                << "PLEASE CHECK YOUR PCAP FILE OR FILEPATH" << std::endl;
      return 1;
  }

  // Checks frame count
  int retVal = 0;
  retVal += TestFrameCount(reader->GetNumberOfFrames(), referenceFilesList.size());

  // Check properties frame by frame
  int nbReferences = referenceFilesList.size();

  // All frames are tested (even the first and the last one)
  // Don't forget to ShowFirstAndLastFrame to generate new test data
  for (int idFrame = 0; idFrame < nbReferences; ++idFrame)
  {
    std::cout << "---------------------" << std::endl
              << "FRAME " << idFrame << " ..." << std::endl
              << "---------------------" << std::endl;

    vtkPolyData* currentFrame = GetCurrentFrame(reader, idFrame);
    vtkPolyData* currentReference = GetCurrentReference(referenceFilesList, idFrame);

    // Check points count
    retVal += TestPointCount(currentFrame, currentReference);

    // Check points position
    retVal += TestPointPositions(currentFrame, currentReference);

    // Check pointData structure
    retVal += TestPointDataStructure(currentFrame, currentReference);

    // Check pointData values
    retVal += TestPointDataValues(currentFrame, currentReference);

    // Check RPM values
    retVal += TestRPMValues(currentFrame, currentReference);
  }

  retVal  += TestNetworkTimeToLidarTime(reader, referenceNetworkTimeToDataTime);

  return  retVal;

}

//-----------------------------------------------------------------------------
int testLidarStream(vtkLidarStream *stream,
                    bool preSend,
                    double preSendSpeed,
                    double speed,
                    const std::string& pcapFileName,
                    const std::string& referenceFileName)
{
  int retVal = 0;
  // get VTP file name from the reference file
  std::vector<std::string> referenceFilesList;
  referenceFilesList = GenerateFileList(referenceFileName);

  const std::string destinationIp = "127.0.0.1";
  const int dataPort = 2368;

  const int preSendWait_us = static_cast<int>(1e6 * 1.0 / preSendSpeed);
  const int sendWait_us = static_cast<int>(1e6 * 1.0 / speed);

  // Case of live correction : Packet are sent a first time to save calibration
  if (preSend)
  {
    try
    {
      stream->Start();
      vvPacketSender sender(pcapFileName, destinationIp, dataPort);
      while (!sender.IsDone())
      {
        sender.pumpPacket();
        boost::this_thread::sleep(boost::posix_time::microseconds(preSendWait_us));
      }
      stream->Stop();
      if (stream->GetNeedsUpdate())
      {
        stream->Update(); // discard frame decoded before next pass
      }
    }
    catch (std::exception& e)
    {
      std::cout << "Caught Exception: " << e.what() << std::endl;
      return 1;
    }
  }


  // Packets are sent, if a new frame is ready it's compare to the associated reference frame
  std::cout << "Sending data... " << std::endl;
  const double startTime = vtkTimerLog::GetUniversalTime();
  double timeLastPacketSent = startTime;
  const double timeout = 1.0;

  stream->Start();
  if (stream->GetInterpreter()->GetIsCalibrated())
  {
    try
    {
      vvPacketSender sender(pcapFileName, destinationIp, dataPort);

      // Limit the number of packets sent to limit the execution time of tests
      unsigned int maxNbPackets = 25000;
      unsigned int nbCurrentPackets = 0;

      int idFrame = 0;
      bool done = false;
      bool tooManyPackets = false;
      bool didTimeout = false;
      while (!done)
      {
        if (!sender.IsDone())
        {
          sender.pumpPacket();
          nbCurrentPackets++;
          timeLastPacketSent = vtkTimerLog::GetUniversalTime();
        }

        boost::this_thread::sleep(boost::posix_time::microseconds(sendWait_us));

        // A new frame is ready
        if (stream->GetNeedsUpdate())
        {
          // Skips the first & last frames. First and last frame aren't complete frames
          // In live mode, we don't skip the last firing belonging to the n-1 frame.
          // In live mode, the last frame is uncomplete.
          std::cout << "---------------------" << std::endl
                    << "FRAME " << idFrame << std::endl
                    << "---------------------" << std::endl;

          stream->Update();

          vtkPolyData* currentFrame = stream->GetOutput();
          vtkPolyData* currentReference = GetCurrentReference(referenceFilesList, idFrame);

          // Check Points count
          retVal += TestPointCount(currentFrame, currentReference);

          // Check Points position
          retVal += TestPointPositions(currentFrame, currentReference);

          // Check PointData structure
          retVal += TestPointDataStructure(currentFrame, currentReference);

          // Check PointData values
          retVal += TestPointDataValues(currentFrame, currentReference);

          // Check RPM values
          // This values are computed differently in stream and reader,
          // so to use reader generated ground-truth, tolerance is generous.
          // Also we do not expect correct RPM on first frame.
          if (idFrame > 0)
          {
            retVal += TestRPMValues(currentFrame, currentReference, 20.0);
          }

          idFrame++;
        }
        tooManyPackets = nbCurrentPackets > maxNbPackets;
        double timeSinceLastPacketSent = vtkTimerLog::GetUniversalTime() - timeLastPacketSent;
        didTimeout = timeSinceLastPacketSent > timeout;
        done =  didTimeout || tooManyPackets;
      }

      if (tooManyPackets)
      {
        std::cout << "PCAP seems to contain too many packets."
                     " Please make it shorter to allow fast tests." << std::endl;
        retVal += 1;
      }

      if (!sender.IsDone())
      {
        std::cout << "Problem when sending data. Received: " << idFrame << " frames." << std::endl;
        retVal += 1;
      }

      retVal += TestFrameCount(idFrame, referenceFilesList.size());
    }
    catch (std::exception& e)
    {
      std::cout << "Caught Exception: " << e.what() << std::endl;
      return 1;
    }
  }
  else
  {
    std::cout << "Test failed: Interpreter is not calibrated." << std::endl;
    retVal += 1;
  }
  stream->Stop();

  double elapsedTime = vtkTimerLog::GetUniversalTime() - startTime;
  std::cout << "Data sent in " << elapsedTime << "s" << std::endl;
  std::cout << "Done." << std::endl;

  return retVal;
}
