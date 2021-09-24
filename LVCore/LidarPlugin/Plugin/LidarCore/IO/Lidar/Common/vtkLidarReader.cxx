#include "vtkLidarReader.h"

#include <sstream>

#include "Common/Network/vtkPacketFileWriter.h"
#include "Common/Network/vtkPacketFileReader.h"
#include "Common/statistics.h"

#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
vtkLidarReader::vtkLidarReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
int vtkLidarReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  if ( port == 0 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }
  if ( port == 1 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
std::string vtkLidarReader::GetSensorInformation(bool shortVersion)
{
  return this->Interpreter->GetSensorInformation(shortVersion);
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetDummyProperty(int)
{
  return this->Modified();
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkLidarReader::GetMTime()
{
  if (this->Interpreter)
  {
    return std::max(this->Superclass::GetMTime(), this->Interpreter->GetMTime());
  }
  return this->Superclass::GetMTime();
}

//-----------------------------------------------------------------------------
int vtkLidarReader::ReadFrameInformation()
{
  this->Open();
  const unsigned char* data = 0;
  unsigned int dataLength = 0;
  bool firstIteration = true;

  // reset the frame catalog to build a new one
  this->FrameCatalog.clear();

  // reset the interpreter parser meta data
  this->Interpreter->ResetParserMetaData();

  if (!this->Reader)
  {
    vtkErrorMacro("Could not open the pcap file");
    return 0;
  }

  // keep track of the file position
  // and the network timestamp of the
  // current udp packet to process
  fpos_t lastFilePosition;
  double lastPacketNetworkTime = 0;
  this->Reader->GetFilePosition(&lastFilePosition);

  while (this->Reader->NextPacket(data, dataLength, lastPacketNetworkTime))
  {
    // This command sends a signal that can be observed from outside
    // and that is used to diplay a Qt progress dialog from Python
    // This progress dialog is not displaying a progress percentage,
    // thus it is ok to pass 0.0
    this->UpdateProgress(0.0);

    // If the current packet is not a lidar packet,
    // skip it and update the file position
    if (!this->Interpreter->IsLidarPacket(data, dataLength))
    {
      this->Reader->GetFilePosition(&lastFilePosition);
      continue;
    }

    // add an index for the first Lidar packet
    if (firstIteration && this->GetInterpreter()->GetFramingMethod() == INTERPRETER_FRAMING)
    {
      // it is possible that the first packet contains 2 frames
      // (end and start of one), and as we rely on the packet header time
      // this 2 frames will have the same timestep. So to avoid that we
      // artificatially move the first timeStep back by one.
      this->FrameCatalog.push_back(this->Interpreter->GetParserMetaData());
      this->FrameCatalog.back().FilePosition = lastFilePosition; // Ensure that the first index point on a real fileposition
      firstIteration = false;
    }

    // Get information about the current packet
    this->Interpreter->PreProcessPacketWrapped(data, dataLength, lastFilePosition,
                                        lastPacketNetworkTime, &this->FrameCatalog);

    this->Reader->GetFilePosition(&lastFilePosition);
  }

  if (this->FrameCatalog.size() == 1)
  {
    vtkErrorMacro("The reader could not parse the pcap file");
  }

  this->NetworkTimeToDataTime = 0.0; // default value if no frames seen
  if (this->FrameCatalog.size() > 0)
  {
      std::vector<double> diffs(this->FrameCatalog.size());
      for (size_t i = 0; i < this->FrameCatalog.size(); i++)
      {
          diffs[i] = this->FrameCatalog[i].FirstPacketDataTime - this->FrameCatalog[i].FirstPacketNetworkTime;
      }
      this->NetworkTimeToDataTime = ComputeMedian(diffs);
  }

  if (this->FrameCatalog.size() == 1)
  {
    vtkErrorMacro("The reader could not parse the pcap file");
  }

  return this->GetNumberOfFrames();
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetTimestepInformation(vtkInformation *info)
{
  if (this->FrameCatalog.size() == 1)
  {
    return;
  }
  size_t numberOfTimesteps = this->FrameCatalog.size();
  std::vector<double> timesteps(numberOfTimesteps);
  double timeOffset = this->GetInterpreter()->GetTimeOffset();
  for (size_t i = 0; i < numberOfTimesteps; ++i)
  {
    if(UsePacketTimeForDisplayTime)
    {
      timesteps[i] = this->FrameCatalog[i].FirstPacketDataTime;
    }
    else
    {
      timesteps[i] = this->FrameCatalog[i].FirstPacketNetworkTime + timeOffset;
    }
  }
  if (this->FrameCatalog.size())
  {
    double* firstTimestepPointer = &timesteps.front();
    double* lastTimestepPointer = &timesteps.back();
    // Remove the first and last frame timestep
    // in case you have less that 3 timstep we will still show the partial
    // frame to avoid showing nothing
    if (!this->ShowFirstAndLastFrame && numberOfTimesteps >= 3)
    {
      firstTimestepPointer++;
      lastTimestepPointer--;
      numberOfTimesteps -= 2;

    }
    double timeRange[2] = { *firstTimestepPointer, *lastTimestepPointer };
    // In order to avoid to display
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), firstTimestepPointer, numberOfTimesteps);
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  else
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarReader)

//-----------------------------------------------------------------------------
void vtkLidarReader::SetFileName(const std::string &filename)
{
  if (filename == this->FileName)
  {
    return;
  }

  this->FileName = filename;
  this->FrameCatalog.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkLidarReader::GetFrame(int frameNumber)
{
  this->Interpreter->ResetCurrentFrame();
  this->Interpreter->ClearAllFramesAvailable();

  if (!this->Reader)
  {
    vtkErrorMacro("GetFrame() called but packet file reader is not open.");
    return 0;
  }
  if (!this->Interpreter->GetIsCalibrated())
  {
    vtkErrorMacro("Calibration data has not been loaded.");
    return 0;
  }

  const unsigned char* data = 0;
  unsigned int dataLength = 0;
  double timeSinceStart = 0;

  // Update the interpreter meta data according to the requested frame
  FrameInformation currInfo= this->FrameCatalog[frameNumber];
  this->Interpreter->SetParserMetaData(this->FrameCatalog[frameNumber]);
  this->Reader->SetFilePosition(&currInfo.FilePosition);

  while (this->Reader->NextPacket(data, dataLength, timeSinceStart))
  {
    // If the current packet is not a lidar packet,
    // skip it and update the file position
    if (!this->Interpreter->IsLidarPacket(data, dataLength))
    {
      continue;
    }

    // Process the lidar packet and check
    // if the required frame is ready
    this->Interpreter->ProcessPacketWrapped(data, dataLength, timeSinceStart);
    if (this->Interpreter->IsNewFrameReady())
    {
      return this->Interpreter->GetLastFrameAvailable();
    }
  }

  this->Interpreter->SplitFrame(true);
  return this->Interpreter->GetLastFrameAvailable();
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkLidarReader::GetFrameForPacketTime(double packetTime)
{
  return this->GetFrame(this->GetFrameIndexForPacketTime(packetTime));
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkLidarReader::GetFrameForDataTime(double dataTime)
{
  return this->GetFrame(this->GetFrameIndexForDataTime(dataTime));
}

//-----------------------------------------------------------------------------
int vtkLidarReader::GetFrameIndexForPacketTime(double packetTime)
{
  // iterating over all timesteps until finding the first one with a greater time value
  auto idx = std::lower_bound(this->FrameCatalog.begin(),
                              this->FrameCatalog.end(),
                              packetTime,
                              [](FrameInformation& fp, double d)
                                { return fp.FirstPacketNetworkTime < d; });

  if (idx == this->FrameCatalog.end())
  {
    vtkErrorMacro("Cannot meet timestep request: " << packetTime);
    return 0;
  }

  auto frameRequested = std::distance(this->FrameCatalog.begin(), idx);
  return static_cast<int>(frameRequested);
}

//-----------------------------------------------------------------------------
int vtkLidarReader::GetFrameIndexForDataTime(double dataTime)
{
  // iterating over all timesteps until finding the first one with a greater time value
  auto idx = std::lower_bound(this->FrameCatalog.begin(),
                              this->FrameCatalog.end(),
                              dataTime,
                              [](FrameInformation& fp, double d)
                                { return fp.FirstPacketDataTime < d; });

  if (idx == this->FrameCatalog.end())
  {
    vtkErrorMacro("Cannot meet timestep request: " << dataTime);
    return 0;
  }

  auto frameRequested = std::distance(this->FrameCatalog.begin(), idx);
  return static_cast<int>(frameRequested);
}

//-----------------------------------------------------------------------------
void vtkLidarReader::Open(bool reassemble)
{
  this->Close();
  this->Reader = new vtkPacketFileReader;

  std::string filterPCAP = "udp";
  if (this->LidarPort != -1)
  {
    filterPCAP += " port " + std::to_string(this->LidarPort);
  }
  if (!this->Reader->Open(this->FileName, filterPCAP.c_str(), reassemble))
  {
    vtkErrorMacro(<< "Failed to open packet file: " << this->FileName << "!\n"
                                                 << this->Reader->GetLastError());
    this->Close();
  }
}

//-----------------------------------------------------------------------------
void vtkLidarReader::Close()
{
  delete this->Reader;
  this->Reader = 0;
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SaveFrame(int startFrame, int endFrame, const std::string &filename)
{
  if (!this->Reader)
  {
    vtkErrorMacro("SaveFrame() called but packet file reader is not open.");
    return;
  }

  if (this->GetInterpreter()->GetFramingMethod()!= INTERPRETER_FRAMING)
  {
    // not implemented yet, because not critical for the client
    vtkErrorMacro("SaveFrame() is only supported if the framing is provided by the interpreter.");
    return;
  }

  vtkPacketFileWriter writer;
  if (!writer.Open(filename))
  {
    vtkErrorMacro("Failed to open packet file for writing: " << filename);
    return;
  }


  // Ensure that frame indexes match between what is effectively shown
  // and what is present inside the PCAP
  size_t numberOfTimesteps = this->FrameCatalog.size();
  if (!this->ShowFirstAndLastFrame && numberOfTimesteps >= 3)
  {
    startFrame++;
    endFrame++;
  }

  // because fpos_t is plateform specific and should not be used for comparaison
  // it's not possible to simply interate from FiePositions[start] to FilePositions[end]
  // we need to detect new frame in the pcap directly once again
  pcap_pkthdr* header = 0;
  const unsigned char* data = 0;
  unsigned int dataLength = 0;
  unsigned int dataHeaderLength = 0;
  double timeSinceStart = 0;
  int currentFrame = startFrame;

  // Explanation for why we need to allow currentFrame to go to endFrame + 1:
  // If '[]' represents a packet, '|' represents the separation between frames,
  // Then the most generic form of a Lidar PCAP is*:
  // [-- incomplete frame --|-- begin frame 0 --]
  // [-- content of frame 0 --]
  // ... many packets ...
  // [-- end frame 0 --|-- begin frame1 --]
  // ... end of the PCAP
  // Here we see that the first separation between two frame happens in the
  // first packet. We do need to see one more separation than the number of
  // packets to write. This is due to the fact that the first incomplete frame
  // is hidden by LidarView.
  // A possible improvement to LidarView would be to not always hide this first
  // frame, depending on a flag set on the reader.
  // *if you are very lucky the first frame will start at the begining of the
  // first packet, and there will be no "incomplete frame".
  //
  // In my test, writing all frames of the PCAP results in a .pcap file exactly
  // identical to the one that is read, if you enable "ShowFirstAndLastFrame".

  this->Reader->SetFilePosition(&this->FrameCatalog[startFrame].FilePosition);

  // Since the PreProcessPacket method of the interpreter can change
  // its internal state, we store and then restore the contained meta
  // data
  FrameInformation storedMetaData = this->Interpreter->GetParserMetaData();

  while (this->Reader->NextPacket(
           data, dataLength, timeSinceStart, &header, &dataHeaderLength)
         && currentFrame <= endFrame + 1) // see explanation above for "+ 1"
  {
    // writing all packets, even those that do not contain lidar frames,
    // such as IMU data or GPS data
    writer.WritePacket(header, data - dataHeaderLength);
    if (this->Interpreter->IsLidarPacket(data, dataLength))
    {
      // we need to count frames and some are split in multiple packets
      // we give timeSinceStart in case Framing Method is not the interpreter one
      bool isNewFrame = this->Interpreter->PreProcessPacketWrapped(data, dataLength, fpos_t(),
                                                                timeSinceStart);

      currentFrame += static_cast<int>(isNewFrame);
      this->UpdateProgress(0.0);
    }
  }
  writer.Close();
  // restore the meta data
  this->Interpreter->SetParserMetaData(storedMetaData);
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetLidarPort(int _arg)
{
  if (this->LidarPort != _arg)
  {
    this->LidarPort = _arg;
    this->FrameCatalog.clear();
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkLidarReader::RequestData(vtkInformation *vtkNotUsed(request),
                                vtkInformationVector **vtkNotUsed(inputVector),
                                vtkInformationVector *outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector);
  vtkTable* calibration = vtkTable::GetData(outputVector, 1);

  vtkInformation* info = outputVector->GetInformationObject(0);

  if (this->FileName.empty())
  {
    vtkErrorMacro("FileName has not been set.");
    return 0;
  }

  if (!this->Interpreter)
  {
    vtkErrorMacro("Interpreter has not been set.");
    return 0;
  }

  if (!this->Interpreter->GetIsCalibrated())
  {
    vtkErrorMacro("The calibration could not be determined from the pcap file!");
    return 0;
  }

  calibration->ShallowCopy(this->Interpreter->GetCalibrationTable());
  if (this->FrameCatalog.size() <= 1) // This mean that the reader did not manage to parser the pcap file
  {
    return 1;
  }

  double timestep = 0.0;
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    timestep = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  int frameRequested = 0;
  if(this->UsePacketTimeForDisplayTime)
  {
    frameRequested = GetFrameIndexForDataTime(timestep);
  }
  else
  {
    frameRequested = GetFrameIndexForPacketTime(timestep);
  }

  // detect frame dropping
  if (this->DetectFrameDropping)
  {
    int step = frameRequested - this->LastFrameProcessed;
    if (step > 1)
    {
      std::stringstream text;
      text << "WARNING : At frame " << std::right << std::setw(6) << frameRequested
           << " Drop " << std::right << std::setw(2) << step-1 << " frame(s)\n";
      vtkWarningMacro( << text.str() );
    }
  }
  this->LastFrameProcessed = frameRequested;

  //! @todo we should no open the pcap file everytime a frame is requested !!!
  this->Open();
  output->ShallowCopy(this->GetFrame(frameRequested));
  this->Close();

  return 1;
}

//-----------------------------------------------------------------------------
int vtkLidarReader::RequestInformation(vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **vtkNotUsed(inputVector),
                                       vtkInformationVector* outputVector)
{
  if (!this->Interpreter)
  {
    vtkErrorMacro("No packet interpreter selected.");
  }

  // load the calibration file only now to allow to set it before the interpreter.
  if (this->Interpreter->GetCalibrationFileName() != this->CalibrationFileName)
  {
    this->Interpreter->SetCalibrationFileName(this->CalibrationFileName);
    this->Interpreter->LoadCalibration(this->CalibrationFileName);
  }

  if (this->Interpreter && !this->FileName.empty())
  {
    this->ReadFrameInformation();
  }
  vtkInformation* info = outputVector->GetInformationObject(0);
  this->SetTimestepInformation(info);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkLidarReader::SetCalibrationFileName(const std::string &filename)
{
  if (filename == this->CalibrationFileName)
  {
    return;
  }

  if (!vtksys::SystemTools::FileExists(filename) ||
    vtksys::SystemTools::FileIsDirectory(filename))
  {
    std::ostringstream errorMessage("Invalid sensor configuration file ");
    errorMessage << filename << ": ";
    if (!vtksys::SystemTools::FileExists(filename))
    {
      errorMessage << "File not found!";
    }
    else
    {
      errorMessage << "It is a directory!";
    }
    vtkErrorMacro(<< errorMessage.str());
    return;
  }
  this->CalibrationFileName = filename;
  this->Modified();
}
