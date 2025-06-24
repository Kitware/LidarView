/*=========================================================================

  Program: LidarView
  Module:  vtkLidarReader.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarReader.h"

// Compliance with vtk's fpos_t policy, needs to be included before any libc header
#include "vtkPacketFilePositionType.h"

#include <vtkCommand.h>
#include <vtkDataObject.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include "statistics.h"
#include "vtkLidarPacketInterpreter.h"
#include "vtkPacketFileHandler.h"
#include "vtkStreamPacketHandler.h"

#include <algorithm>
#include <sstream>

namespace
{
constexpr double SHOW_FRAME_TOLERANCE = 0.01; // 10ms
constexpr double PARTIAL_FRAME_TOLERANCE = 0.95;
constexpr size_t SAMPLE_SIZE = 10;

//-----------------------------------------------------------------------------
bool IsPartialFrame(double frameDuration, double reference)
{
  return frameDuration < (reference * ::PARTIAL_FRAME_TOLERANCE);
}

//-----------------------------------------------------------------------------
double ComputeDurationMean(std::vector<double>::const_iterator start, size_t size)
{
  std::vector<double>::const_iterator end = start + size;
  std::vector<double> duration(size - 1);
  auto diff = [](double first, double second) { return second - first; };
  std::transform(start, end - 1, start + 1, duration.begin(), diff);
  return std::accumulate(duration.cbegin(), duration.cend(), 0.) / (size - 1);
}
}

//-----------------------------------------------------------------------------
class vtkLidarReader::vtkInternals
{
public:
  struct LidarFrame
  {
    /**
     * Position of the first packet of the given frame
     */
    vtkPcapIdxType FilePosition;

    /**
     * To be agnostic to the underlying data, we rely on the first packet time step to determine
     * the time of frame. The packet time step has no relation with the timesteps that are in the
     * payload of the packet. It's contained in the header, and indicate when a packet has been
     * received
     */
    double NetworkTime;

    /**
     * Timestamp of data contained in the first packet
     */
    double LidarTime;
  };
  std::vector<LidarFrame> FramesIndex;
  vtkSmartPointer<vtkPacketFileHandler> Reader;
  bool NeedsReIndexing = true;
  double LastFrameRequested = 0;
  // The following are for EmulatedTime hiding frames
  bool EmptyFrameUpdate = false;
  bool ShouldRefreshEmptyFrame = false;
  unsigned long ReaderObserverId = 0;

  //----------------------------------------------------------------------------
  double ComputeNetworkTimeToDataTime()
  {
    if (FramesIndex.empty())
    {
      return 0.;
    }
    std::vector<double> diffs(this->FramesIndex.size());
    auto computeDiff = [](const LidarFrame& frame) { return frame.LidarTime - frame.NetworkTime; };
    std::transform(
      this->FramesIndex.cbegin(), this->FramesIndex.cend(), diffs.begin(), computeDiff);
    return ComputeMedian(diffs);
  }

  //----------------------------------------------------------------------------
  std::vector<double> GetFramesTimeSteps(const int& type)
  {
    std::vector<double> timesteps(this->FramesIndex.size());

    auto extractor = [&type](const LidarFrame& frame)
    {
      switch (type)
      {
        case TimeType::USE_LIDAR_TIME:
          return frame.LidarTime;

        case TimeType::USE_NETWORK_TIME:
        default:
          return frame.NetworkTime;
      }
    };
    std::transform(
      this->FramesIndex.cbegin(), this->FramesIndex.cend(), timesteps.begin(), extractor);
    return timesteps;
  }

  //----------------------------------------------------------------------------
  /**
   * Remove any partial first or last frame.
   * A frame is deemed partial if its duration falls below the reference duration
   * by a tolerance of 5%.
   * The reference duration is determined as the average duration of the 10
   * succeeding or preceding frames.
   */
  void HidePartialFrames(double lastNetworkTime, int timeType)
  {
    std::vector<double> timesteps = this->GetFramesTimeSteps(timeType);
    if (timesteps.size() <= 3)
    {
      return;
    }
    const size_t sampleSize = std::min(timesteps.size() - 1, ::SAMPLE_SIZE);
    const double firstFrameDuration = timesteps[1] - timesteps[0];
    const double lastFrameDuration = lastNetworkTime - timesteps.back();

    const double meanFirstFrames = ::ComputeDurationMean(timesteps.cbegin() + 1, sampleSize);
    const double meanLastFrames = ::ComputeDurationMean(timesteps.cend() - sampleSize, sampleSize);

    if (::IsPartialFrame(firstFrameDuration, meanFirstFrames))
    {
      this->FramesIndex.erase(this->FramesIndex.begin());
    }
    if (::IsPartialFrame(lastFrameDuration, meanLastFrames))
    {
      this->FramesIndex.pop_back();
    }
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarReader);

//-----------------------------------------------------------------------------
vtkLidarReader::vtkLidarReader()
  : Internals(new vtkLidarReader::vtkInternals())
{
  this->Internals->Reader = vtkSmartPointer<vtkPacketFileHandler>::New();
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//------------------------------------------------------------------------------
vtkLidarReader::~vtkLidarReader()
{
  this->SetLidarInterpreter(nullptr);
}

//-----------------------------------------------------------------------------
int vtkLidarReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
std::string vtkLidarReader::GetSensorInformation(bool shortVersion)
{
  return this->LidarInterpreter->GetSensorInformation(shortVersion);
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::GetNeedsUpdate(double time)
{
  if (this->Internals->FramesIndex.empty())
  {
    return false;
  }

  bool needUpdate = false;
  double* ranges = this->Superclass::GetTimeRange();
  if (ranges[0] - ::SHOW_FRAME_TOLERANCE <= time && time <= ranges[1] + ::SHOW_FRAME_TOLERANCE)
  {
    needUpdate = this->Superclass::GetNeedsUpdate(time);
    this->Internals->ShouldRefreshEmptyFrame = true;
  }
  else
  {
    // Avoid refreshing empty frame for each update
    if (this->Internals->ShouldRefreshEmptyFrame)
    {
      this->vtkAlgorithm::Modified();
      this->Internals->EmptyFrameUpdate = true;
      this->Internals->ShouldRefreshEmptyFrame = false;
    }
  }
  return needUpdate || this->Internals->EmptyFrameUpdate;
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::BuildFramesIndex()
{
  if (!this->Open())
  {
    return false;
  }
  // reset the frame catalog to build a new one
  this->Internals->FramesIndex.clear();

  vtkPcapIdxType lastFilePosition;
  double networkPacketTime = 0;
  this->Internals->Reader->GetFilePosition(&lastFilePosition);

  bool isFirstPacket = true;
  vtkLidarReader::vtkInternals::LidarFrame currentFrame;

  while (this->ReadNextPacket(networkPacketTime))
  {
    const std::vector<uint8_t>& payload = this->GetPayload();

    // This command sends a signal that can be observed from outside
    // and that is used to diplay a Qt progress dialog from Python
    // This progress dialog is not displaying a progress percentage,
    // thus it is ok to pass 0.0
    this->UpdateProgress(0.0);

    // If the current packet is not a lidar packet,
    // skip it and update the file position
    if (!this->LidarInterpreter->IsLidarPacket(payload.data(), payload.size()))
    {
      this->Internals->Reader->GetFilePosition(&lastFilePosition);
      continue;
    }

    // Get information about the current packet
    double lidarDataTime = 0.;
    bool splitFrame = this->LidarInterpreter->PreProcessPacketWrapped(
      payload.data(), payload.size(), networkPacketTime, lidarDataTime);

    if (isFirstPacket)
    {
      currentFrame = { lastFilePosition, networkPacketTime, lidarDataTime };
    }
    if (splitFrame)
    {
      if (!isFirstPacket)
      {
        this->Internals->FramesIndex.emplace_back(currentFrame);
      }
      currentFrame = { lastFilePosition, networkPacketTime, lidarDataTime };
    }
    isFirstPacket = false;

    this->Internals->Reader->GetFilePosition(&lastFilePosition);
  }
  // If isFirstPacket is still true, the interpreter failed to find one at least one valid packet.
  if (!isFirstPacket)
  {
    this->Internals->FramesIndex.emplace_back(currentFrame);
  }

  if (!this->ShowPartialFrames && this->Internals->FramesIndex.size() >= 3)
  {
    this->Internals->HidePartialFrames(networkPacketTime, this->DisplayTimeType);
  }

  this->Close();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::ReadFrame(size_t index, vtkPolyData* output)
{
  if (!this->Open())
  {
    vtkErrorMacro("Could not open the packet file reader.");
    return false;
  }
  this->LidarInterpreter->ResetCurrentFrame();
  this->LidarInterpreter->ClearAllFramesAvailable();

  if (!this->LidarInterpreter->GetIsInitialized())
  {
    vtkErrorMacro("Calibration data has not been loaded.");
    return 0;
  }

  double timeSinceStart = 0;

  // Update the interpreter meta data according to the requested frame
  auto& currentFrame = this->Internals->FramesIndex[index];
  this->Internals->Reader->SetFilePosition(&currentFrame.FilePosition);

  while (this->ReadNextPacket(timeSinceStart))
  {
    const std::vector<uint8_t>& payload = this->GetPayload();
    // If the current packet is not a lidar packet,
    // skip it and update the file position
    if (!this->LidarInterpreter->IsLidarPacket(payload.data(), payload.size()))
    {
      continue;
    }

    // Process the lidar packet and check
    // if the required frame is ready
    this->LidarInterpreter->ProcessPacketWrapped(payload.data(), payload.size(), timeSinceStart);
    if (this->LidarInterpreter->IsNewFrameReady())
    {
      output->ShallowCopy(this->LidarInterpreter->GetLastFrameAvailable());
      this->Close();
      return true;
    }
  }

  this->LidarInterpreter->SplitFrame(true);

  output->ShallowCopy(this->LidarInterpreter->GetLastFrameAvailable());
  this->Close();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::Open()
{
  std::vector<int> ports;
  if (this->LidarPort != -1)
  {
    ports.emplace_back(this->LidarPort);
  }
  return this->Open(ports);
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::Open(bool vtkNotUsed(reassemble))
{
  return this->Open();
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::Open(std::vector<int> ports)
{
  if (this->FileName.empty())
  {
    vtkErrorMacro(<< "No FileName set!");
    return false;
  }

  std::string filterPCAP = vtkStreamPacketHandler::BuildPCAPFilter("udp", "", ports);

  if (!this->Internals->Reader->Open(this->FileName, filterPCAP))
  {
    vtkErrorMacro(<< "Failed to open packet file: " << this->FileName << "!");
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::Open(std::vector<int> ports, bool vtkNotUsed(reassemble))
{
  return this->Open(ports);
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::ReadNextPacket(double& timeSinceStart)
{
  bool ret = this->Internals->Reader->ReadNextPacket();
  timeSinceStart = this->Internals->Reader->GetTimestamp();
  return ret;
}

//-----------------------------------------------------------------------------
bool vtkLidarReader::ReadNextPacket(const unsigned char*& data,
  unsigned int& dataLength,
  double& timeSinceStart)
{
  bool ret = this->ReadNextPacket(timeSinceStart);
  const std::vector<uint8_t>& payload = this->GetPayload();
  data = payload.data();
  dataLength = payload.size();
  return ret;
}

//-----------------------------------------------------------------------------
const std::vector<uint8_t>& vtkLidarReader::GetPayload()
{
  return this->Internals->Reader->GetPayload();
}

//-----------------------------------------------------------------------------
void vtkLidarReader::Close()
{
  if (this->Internals->Reader->IsOpen())
  {
    this->Internals->Reader->Close();
  }
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SaveFrames(unsigned int startFrame,
  unsigned int endFrame,
  const std::string& filename)
{
  if (!this->Open())
  {
    vtkErrorMacro("SaveFrame() called but could not open the packet file reader.");
    return;
  }

  if (this->GetLidarInterpreter()->GetFramingMethod() != INTERPRETER_FRAMING)
  {
    // not implemented yet, because not critical for the client
    vtkErrorMacro("SaveFrame() is only supported if the framing is provided by the interpreter.");
    return;
  }

  size_t frameNb = this->Internals->FramesIndex.size();
  if (startFrame >= frameNb || endFrame >= frameNb || startFrame > endFrame)
  {
    vtkErrorMacro("Could not save pcap with such bounds.");
    return;
  }

  vtkPcapIdxType& startPosition = this->Internals->FramesIndex[startFrame].FilePosition;
  const double endNetworkTime = endFrame >= frameNb - 2
    ? std::numeric_limits<double>::max()
    : this->Internals->FramesIndex[endFrame + 1].NetworkTime;

  // writing all packets, even those that do not contain lidar frames,
  // such as IMU data or GPS data
  this->Internals->Reader->WritePackets(filename, &startPosition, endNetworkTime);

  this->Close();
}

//-----------------------------------------------------------------------------
int vtkLidarReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  if (!this->LidarInterpreter)
  {
    vtkErrorMacro("No packet interpreter selected.");
    return 0;
  }

  if (!this->LidarInterpreter->GetIsInitialized())
  {
    this->LidarInterpreter->Initialize();
    this->ResetFrameIndexes();
  }

  if (this->Internals->NeedsReIndexing && !this->FileName.empty())
  {
    this->BuildFramesIndex();
    this->Internals->NeedsReIndexing = false;
  }

  vtkInformation* info = outputVector->GetInformationObject(0);
  if (!this->Internals->FramesIndex.empty())
  {
    std::vector<double> timesteps = this->Internals->GetFramesTimeSteps(this->DisplayTimeType);
    bool isSorted = std::is_sorted(timesteps.cbegin(), timesteps.cend());
    if (!isSorted)
    {
      unsigned int oldSize = timesteps.size();
      double prev = timesteps.front();
      auto toRemove = [&prev](double value)
      {
        bool remove = value < prev;
        prev = std::max(prev, value);
        return remove;
      };
      auto newEnd = std::remove_if(timesteps.begin(), timesteps.end(), toRemove);
      timesteps.erase(newEnd, timesteps.end());
      vtkWarningMacro(<< "The pcap timestamps are not sorted in ascending order, "
                      << oldSize - timesteps.size()
                      << " frames were removed but frame indexing could still be incorrect."
                      << " Changing \"Display Packet Time\" parameter might solve that issue.");
    }

    double timeOffset = this->GetLidarInterpreter()->GetTimeOffset();
    if (timeOffset != 0)
    {
      auto addOffset = [&timeOffset](double time) { return time + timeOffset; };
      std::transform(timesteps.cbegin(), timesteps.cend(), timesteps.begin(), addOffset);
    }

    double timeRange[2];
    timeRange[0] = timesteps.front();
    timeRange[1] = timesteps.back();
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timesteps.data(), timesteps.size());
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  else
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkLidarReader::RequestUpdateExtent(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  int ret = this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
  if (this->Internals->EmptyFrameUpdate)
  {
    double* ranges = this->Superclass::GetTimeRange();
    double backFrame = ranges[1] + ::SHOW_FRAME_TOLERANCE * 2;
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), backFrame);
    this->Internals->EmptyFrameUpdate = false;
  }
  return ret;
}

//-----------------------------------------------------------------------------
int vtkLidarReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector);
  vtkTable* calibration = vtkTable::GetData(outputVector, 1);

  vtkInformation* info = outputVector->GetInformationObject(0);

  if (!this->LidarInterpreter)
  {
    vtkErrorMacro("Interpreter has not been set.");
    return 0;
  }

  if (!this->LidarInterpreter->GetIsInitialized())
  {
    vtkErrorMacro("The calibration could not be determined from the pcap file, and no valid "
                  "calibration file was provided !");
    return 0;
  }

  calibration->ShallowCopy(this->LidarInterpreter->GetCalibrationTable());
  // This mean that the reader did not manage to parser the pcap file
  if (this->Internals->FramesIndex.empty())
  {
    vtkErrorMacro("The parsing of the pcap file failed. Consider attempting with another "
                  "calibration file or a different lidar model.");
    return 0;
  }

  double requestedTime = 0.0;
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    requestedTime = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  double realRequestedTime = requestedTime - this->GetLidarInterpreter()->GetTimeOffset();
  std::vector<double> timesteps = this->Internals->GetFramesTimeSteps(this->DisplayTimeType);

  // Hide output if requested timestep falls outside the available range of timesteps.
  // Note that the max precision of realRequestedTime is 0.01s
  if (realRequestedTime < timesteps.front() - ::SHOW_FRAME_TOLERANCE ||
    realRequestedTime > timesteps.back() + ::SHOW_FRAME_TOLERANCE)
  {
    return 1;
  }
  auto indexRequested = std::distance(
    timesteps.begin(), std::lower_bound(timesteps.begin(), timesteps.end(), realRequestedTime));

  if (this->DetectFrameDropping)
  {
    int step = indexRequested - this->Internals->LastFrameRequested;
    if (step > 1)
    {
      std::stringstream text;
      text << "WARNING : At frame " << std::right << std::setw(6) << indexRequested << ", drop "
           << std::right << std::setw(2) << step - 1 << " frame(s)\n";
      vtkWarningMacro(<< text.str());
    }
  }
  this->Internals->LastFrameRequested = indexRequested;

  return this->ReadFrame(indexRequested, output);
}

//-----------------------------------------------------------------------------
double vtkLidarReader::GetNetworkTimeToDataTime()
{
  return this->Internals->ComputeNetworkTimeToDataTime();
}

//-----------------------------------------------------------------------------
void vtkLidarReader::ResetFrameIndexes()
{
  this->Internals->NeedsReIndexing = true;
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetFileName(const std::string& filename)
{
  if (filename != this->FileName)
  {
    this->FileName = filename;
    this->Modified();
    this->ResetFrameIndexes();
  }
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetLidarPort(int port)
{
  if (this->LidarPort != port)
  {
    this->LidarPort = port;
    this->Modified();
    this->ResetFrameIndexes();
  }
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetShowPartialFrames(bool show)
{
  if (this->ShowPartialFrames != show)
  {
    this->ShowPartialFrames = show;
    this->Modified();
    this->ResetFrameIndexes();
  }
}

//----------------------------------------------------------------------------
void vtkLidarReader::OnInterpreterModifiedEvent()
{
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetLidarInterpreter(vtkLidarPacketInterpreter* interpreter)
{
  if (this->LidarInterpreter == interpreter)
  {
    return;
  }

  if (this->LidarInterpreter)
  {
    this->LidarInterpreter->RemoveObserver(this->Internals->ReaderObserverId);
  }
  vtkSetObjectBodyMacro(LidarInterpreter, vtkLidarPacketInterpreter, interpreter);
  if (this->LidarInterpreter)
  {
    this->Internals->ReaderObserverId = this->LidarInterpreter->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkLidarReader::OnInterpreterModifiedEvent);
  }
}

//------------------------------------------------------------------------------
vtkPolyData* vtkLidarReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkLidarReader::GetOutput(int port)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(port));
}

//------------------------------------------------------------------------------
void vtkLidarReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->LidarInterpreter)
  {
    this->LidarInterpreter->PrintSelf(os, indent);
  }
}
