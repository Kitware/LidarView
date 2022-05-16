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

#include <chrono>
#include <sstream>

#include <thread>

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

  vtkPolyData* currentFrame = vtkPolyData::SafeDownCast(HDLsource->GetOutput());

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
    vtkAbstractArray* currentReferenceArray = currentReferencePointData->GetAbstractArray(idArray);
    std::string currentReferenceArrayName = currentReferenceArray->GetName();
    vtkAbstractArray* currentFrameArray = currentFramePointData->GetAbstractArray(currentReferenceArrayName.c_str());

    if (currentFrameArray == nullptr)
    {
      std::cerr << "failed : Could not find an array named " << currentReferenceArrayName << std::endl;
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
int CompareAbstractArray(vtkAbstractArray* currentArray, vtkAbstractArray* referenceArray)
{

  if(currentArray->GetNumberOfValues() != referenceArray->GetNumberOfValues())
  {
    std::cerr << "number of values does not match" << std::endl;
    return 1;
  }

  for(int idValue = 0; idValue < currentArray->GetNumberOfValues(); idValue++)
  {
    vtkVariant currentValue = currentArray->GetVariantValue(idValue);
    vtkVariant referenceValue = referenceArray->GetVariantValue(idValue);

    if(currentValue.IsArray() && referenceValue.IsArray())
    {
      vtkAbstractArray* currentValueAsAbstractArray = currentValue.ToArray();
      vtkAbstractArray* referenceValueAsAbstractArray = referenceValue.ToArray();
      return CompareAbstractArray(currentValueAsAbstractArray, referenceValueAsAbstractArray);
    }

    else if(currentValue.IsString() && referenceValue.IsString())
    {
      vtkStdString currentValueAsString = currentValue.ToString();
      vtkStdString referenceValueAsString = referenceValue.ToString();
      if(currentValueAsString.compare(referenceValueAsString) == 1)
      {
        std::cerr << "failed : Tuples " << idValue << " doesn't match for array "
                  << referenceArray->GetName() << ". Expected "
                  << referenceValueAsString << ", got " << currentValueAsString
                  << std::endl;
        return 1;
      }
    }

    else if(currentValue.IsNumeric() && referenceValue.IsNumeric())
    {
      double currentDoubleValue = currentValue.ToDouble();
      double referenceDoubleValue = referenceValue.ToDouble();
      if(abs(currentDoubleValue - referenceDoubleValue) > 1e-12)
      {
        if ((referenceValue.ToTypeInt64() - currentValue.ToTypeInt64()) % 3600 * 1e6 == 0)
        {
          std::cerr << "Tuples converted in int64_t are equal up to 3600 * 1e6 " << std::endl;
        }

        std::cerr << "failed : Tuples " << idValue << " doesn't match for array "
                  << referenceArray->GetName() << ". Expected "
                  << referenceDoubleValue << ", got " << currentDoubleValue
                  << std::endl;

        return 1;
      }
    }

    else
    {
      std::cerr << "failed : Tuples " << idValue << " doesn't match for array "
                << referenceArray->GetName() << ". They don't have the same type"
                << std::endl;
      return 1;
    }
  }
  return 0;
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
    vtkAbstractArray* currentFrameArray = currentFramePointData->GetAbstractArray(arrayName);
    vtkAbstractArray* currentReferenceArray = currentReferencePointData->GetAbstractArray(arrayName);
    if(CompareAbstractArray(currentFrameArray, currentReferenceArray))
    {
      return 1;
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
int CheckCurrentFrame(vtkPolyData* currentFrame, vtkPolyData* currentReference)
{
  int retVal = 0;

  // Check Points count
  retVal += TestPointCount(currentFrame, currentReference);

  // Check Points position
  retVal += TestPointPositions(currentFrame, currentReference);

  // Check PointData structure
  retVal += TestPointDataStructure(currentFrame, currentReference);

  // Check PointData values
  retVal += TestPointDataValues(currentFrame, currentReference);
  return retVal;
}

//-----------------------------------------------------------------------------
int testLidarReader(vtkLidarReader *reader,
                    double referenceNetworkTimeToDataTime,
                    const std::string& referenceFileName,
                    const double FrequencyReference_Hz)
{
  // get VTP file name from the reference file
  std::vector<std::string> referenceFilesList;
  referenceFilesList = GenerateFileList(referenceFileName);

  // update the reader to have access to the frame
  reader->Update();

  std::vector<std::chrono::nanoseconds> durations;

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

    // Get The current frame and compute the duration of its processing
    auto start = std::chrono::high_resolution_clock::now();
    vtkPolyData* currentFrame = GetCurrentFrame(reader, idFrame);
    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    durations.push_back(duration);

    // Get the current reference to compare the current frame
    vtkPolyData* currentReference = GetCurrentReference(referenceFilesList, idFrame);
    retVal += CheckCurrentFrame(currentFrame, currentReference);
  }

  // Check the frequency of the processing of a frame
  if(durations.size() > 0)
  {
    std::cout << "Frequency : \t";

    // Compute the mean of all duration
    double mean_s = durations[0].count() * 1e-9;
    for(unsigned int i = 1; i < durations.size(); i++)
    {
      mean_s = mean_s + (durations[i].count() * 1e-9);
    }
    mean_s = mean_s / durations.size();
    if(mean_s == 0)
    {
      std::cerr << "failed : The mean duration of frame processing is equal to 0 s " << std::endl;
      return 1;
    }

    double FrameFrequency_Hz = 1/mean_s;
    if((FrameFrequency_Hz) >= FrequencyReference_Hz )
    {
      std::cout << "passed , expected" << FrequencyReference_Hz
                << " Hz, got : " << (FrameFrequency_Hz) << "Hz." << std::endl;
    }
    else
    {
      std::cerr << "failed : The processing of the frame is to slow. Expected : "
          << FrequencyReference_Hz << " Hz, got : " << (FrameFrequency_Hz) << "Hz." << std::endl;

      retVal += 1;
    }
  }

  retVal  += TestNetworkTimeToLidarTime(reader, referenceNetworkTimeToDataTime);
  return  retVal;

}

//-----------------------------------------------------------------------------
int testLidarStream(vtkLidarStream *stream,
                    bool preSend,
                    double vtkNotUsed(preSendSpeed),
                    double vtkNotUsed(speed),
                    const std::string &pcapFileName,
                    const std::string &referenceFileName,
                    bool vtkNotUsed(testLastFrame))
{
  return testLidarStream(stream, pcapFileName, referenceFileName, preSend);
}

//-----------------------------------------------------------------------------
int testLidarStream(vtkLidarStream *stream,
                    const std::string& pcapFileName,
                    const std::string& referenceFileName,
                    bool preSend)
{
  if(!stream)
  {
    std::cout << "Stream is nullptr" << std::endl;
    return 1;
  }

  int retVal = 0;
  // get VTP file name from the reference file
  std::vector<std::string> referenceFilesList;
  referenceFilesList = GenerateFileList(referenceFileName);

  // Set the value for sending packets
  std::string destinationIp = "127.0.0.1";
  // If the stream listen in multicast, packets should be sent to the right multicast adress
  if(stream->GetMulticastAddress() != "")
  {
    destinationIp = stream->GetMulticastAddress();
  }

  // Set the dataPort where the packets are sent to the same port the stream listen to
  const int dataPort = stream->GetListeningPort();

  auto interpreter = vtkLidarPacketInterpreter::SafeDownCast(stream->GetInterpreter());

  if(!interpreter)
  {
    std::cout << "Interpreter is nullptr" << std::endl;
    return 1;
  }

  // Case of live correction : Packet are sent a first time to save calibration
  if (preSend)
  {
    stream->Start();
    vvPacketSender sender(pcapFileName, destinationIp, dataPort);
    bool isOk = sender.sendAllPackets();
    stream->Stop();
    if (stream->GetNeedsUpdate())
    {
      stream->Update(); // discard frame decoded before next pass
    }
    if (!isOk)
    {
      return 1;
    }
    interpreter->ClearAllFramesAvailable();
    interpreter->ResetParserMetaData();
  }

  stream->Start();
  if (interpreter->GetIsCalibrated())
  {
    retVal += SendAndTestAllFrames(stream, interpreter, referenceFilesList, pcapFileName, destinationIp, dataPort);
    stream->Stop();
  }
  else
  {
    std::cout << "Test failed: Interpreter is not calibrated." << std::endl;
    retVal += 1;
  }
  std::cout << "Done." << std::endl;

  return retVal;
}

//-----------------------------------------------------------------------------
int SendAndTestAllFrames(vtkLidarStream *stream, vtkLidarPacketInterpreter* interpreter,
                         std::vector<std::string> referenceFilesList,
                         const std::string& pcapFileName,
                         std::string destinationIp, int dataPort)
{
  int retVal = 0;

  // Packets are sent, if a new frame is ready it's compare to the associated reference frame
  std::cout << "Sending data... " << std::endl;
  unsigned int idFrame = 0;

  std::function<void()> testFrame = [&](){
    // A new frame is ready
    if (stream->GetNeedsUpdate())
    {
      stream->Update();

      vtkPolyData* currentFrame = vtkPolyData::SafeDownCast(stream->GetOutput());
      vtkPolyData* currentReference = GetCurrentReference(referenceFilesList, idFrame);

      retVal += CheckCurrentFrame(currentFrame, currentReference);

      idFrame++;
    }
  };
  vvPacketSender sender(pcapFileName, destinationIp, dataPort);

  // The packet are at 15% of the real speed. This is because our osx and windows buildboots
  // aren't good enought for handling 100%.
  bool isOk = sender.sendAllPackets(0.15, 0, testFrame);
  if (!isOk)
  {
      std::cout << "Test failed: Error while sending the packets." << std::endl;
      return 1;
  }

  // Wait a bit for every packet to arrive and check if a last complete frame is available.
  std::this_thread::sleep_for(std::chrono::seconds(1));
  testFrame();

  // check for a potential uncomplete last frame.
  if (idFrame < referenceFilesList.size())
   {
     interpreter->SplitFrame(true);

     vtkPolyData* currentFrame = interpreter->GetLastFrameAvailable();
     vtkPolyData* currentReference = GetCurrentReference(referenceFilesList, idFrame);

     // Do not check for the last RPM value as the frame is incomplete.
     retVal += CheckCurrentFrame(currentFrame, currentReference);

     idFrame++;
   }

   retVal += TestFrameCount(idFrame, referenceFilesList.size());

  return retVal;
}

//-----------------------------------------------------------------------------
int TestLidarMulticast(vtkLidarPacketInterpreter* interpreter,
                       const std::string& pcapFileName,
                       const std::string& referenceFileName,
                       bool shouldPreSend, int dataPort,
                       const std::string correctionFileName)
{
  // return value indicate if the test passed
  int retVal = 0;

  vtkSmartPointer<vtkLidarStream> LidarStream = vtkSmartPointer<vtkLidarStream>::New();
  LidarStream->SetLidarInterpreter(interpreter);
  LidarStream->SetCalibrationFileName(correctionFileName);
  LidarStream->SetListeningPort(dataPort);
  LidarStream->SetIsCrashAnalysing(false);
  LidarStream->SetIsForwarding(false);
  LidarStream->SetMulticastAddress("224.0.0.5");
  retVal += testLidarStream(LidarStream.Get(), pcapFileName, referenceFileName, shouldPreSend);

  return retVal;
}

//-----------------------------------------------------------------------------
int TestLidarForwarding(vtkLidarPacketInterpreter* interpreter1,
                        vtkLidarPacketInterpreter* interpreter2,
                        const std::string& pcapFileName,
                        const std::string& referenceFileName,
                        bool shouldPreSend, int dataPort,
                        const std::string correctionFileName)
{

  vtkSmartPointer<vtkLidarStream> LidarStream1 = vtkSmartPointer<vtkLidarStream>::New();
  LidarStream1->SetLidarInterpreter(interpreter1);
  LidarStream1->SetCalibrationFileName(correctionFileName);
  LidarStream1->SetListeningPort(dataPort);
  LidarStream1->SetIsCrashAnalysing(false);
  LidarStream1->SetForwardedPort(dataPort + 1);
  LidarStream1->SetForwardedIpAddress("127.0.0.1");
  LidarStream1->SetIsForwarding(true);

  vtkSmartPointer<vtkLidarStream> LidarStream2 = vtkSmartPointer<vtkLidarStream>::New();
  LidarStream2->SetLidarInterpreter(interpreter2);
  LidarStream2->SetCalibrationFileName(correctionFileName);
  LidarStream2->SetListeningPort(dataPort + 1);

  int retVal = 0;
  // get VTP file name from the reference file
  std::vector<std::string> referenceFilesList;
  referenceFilesList = GenerateFileList(referenceFileName);

  // Set the value for sending packets
  std::string destinationIp = "127.0.0.1";

  // Case of live correction : Packet are sent a first time to save calibration
  if (shouldPreSend)
  {
    LidarStream1->Start();
    LidarStream2->Start();
    vvPacketSender sender(pcapFileName, destinationIp, dataPort);
    bool isOk = sender.sendAllPackets();
    LidarStream1->Stop();
    LidarStream2->Stop();

    if (LidarStream1->GetNeedsUpdate())
    {
      LidarStream1->Update(); // discard frame decoded before next pass
    }
    if (LidarStream2->GetNeedsUpdate())
    {
      LidarStream2->Update(); // discard frame decoded before next pass
    }
    if (!isOk)
    {
      return 1;
    }
    interpreter1->ClearAllFramesAvailable();
    interpreter1->ResetParserMetaData();
    interpreter2->ClearAllFramesAvailable();
    interpreter2->ResetParserMetaData();
  }

  LidarStream1->Start();
  LidarStream2->Start();

  if (interpreter2->GetIsCalibrated())
  {
    // Send all data on DataPort (listen by LidarStream1) and check the result of lidarStream2
    retVal += SendAndTestAllFrames(LidarStream2, interpreter2, referenceFilesList, pcapFileName, destinationIp, dataPort);

    LidarStream1->Stop();
    LidarStream2->Stop();
  }
  else
  {
    std::cout << "Test failed: Interpreter is not calibrated." << std::endl;
    retVal += 1;
  }
  std::cout << "Done." << std::endl;

  return retVal;
}

//-----------------------------------------------------------------------------
int TestLidarRecording(vtkLidarPacketInterpreter* interpreter1,
                       vtkLidarPacketInterpreter* interpreter2,
                       const std::string& pcapFileName,
                       const std::string& referenceFileName,
                       bool shouldPreSend, int dataPort,
                       const std::string correctionFileName)
{
  // return value indicate if the test passed
  int retVal = 0;

  // Create temporary file to test the recording (will be deleted)
  size_t extentionIndex = pcapFileName.find_last_of(".");
  std::string pcapName = pcapFileName.substr(0, extentionIndex);
  std::string temporaryFile = pcapName + "-record.pcap"; // ! will be deleted

  // Test the original pcap and record it to a temporary file
  vtkSmartPointer<vtkLidarStream> LidarStream1 = vtkSmartPointer<vtkLidarStream>::New();
  LidarStream1->SetLidarInterpreter(interpreter1);
  LidarStream1->SetCalibrationFileName(correctionFileName);
  LidarStream1->SetListeningPort(dataPort);
  LidarStream1->StartRecording(temporaryFile); // WIP THIS API CHANGED
  retVal += testLidarStream(LidarStream1.Get(), pcapFileName, referenceFileName, shouldPreSend);
  LidarStream1->StopRecording();

  // Send and test the recorded pcap
  vtkSmartPointer<vtkLidarStream> LidarStream2 = vtkSmartPointer<vtkLidarStream>::New();
  LidarStream2->SetLidarInterpreter(interpreter2);
  LidarStream2->SetCalibrationFileName(correctionFileName);
  LidarStream2->SetListeningPort(dataPort);
  retVal += testLidarStream(LidarStream2.Get(), temporaryFile, referenceFileName, shouldPreSend);

  std::remove(temporaryFile.c_str());

  return retVal;
}

//-----------------------------------------------------------------------------
bool GetFrameTimeRange(vtkPolyData* frame, double& firstTime, double& lastTime)
{
  if (frame == nullptr)
  {
    return false;
  }

  vtkDataArray* timestamps = frame->GetPointData()->GetArray("timestamp");
  if (timestamps == nullptr
      || timestamps->GetNumberOfComponents() != 1
      || timestamps->GetNumberOfTuples() < 1)
  {
    return false;
  }

  firstTime = 1e-6 * timestamps->GetTuple1(0);
  lastTime = 1e-6 * timestamps->GetTuple1(timestamps->GetNumberOfTuples() - 1);
  return true;
}
