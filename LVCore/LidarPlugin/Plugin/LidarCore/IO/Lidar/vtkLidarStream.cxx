//=========================================================================
//
// Copyright 2018 Kitware, Inc.
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

#include "vtkLidarStream.h"

#include <sstream>

#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarStream)

//-----------------------------------------------------------------------------
int vtkLidarStream::FillOutputPortInformation(int port, vtkInformation* info)
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
std::string vtkLidarStream::GetSensorInformation(bool shortVersion)
{
  return this->GetLidarInterpreter()->GetSensorInformation(shortVersion);
}

//-----------------------------------------------------------------------------
void vtkLidarStream::SetCalibrationFileName(const std::string &filename)
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

//-----------------------------------------------------------------------------
void vtkLidarStream::SetDummyProperty(int)
{
  return this->Modified();
}

//-----------------------------------------------------------------------------
vtkLidarStream::vtkLidarStream()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
vtkLidarStream::~vtkLidarStream()
{
  // see the explanation about why this is needed in vtkStream::~vtkStream
  this->Stop();
}

//-----------------------------------------------------------------------------
int vtkLidarStream::Calibrate()
{
  if (!this->GetLidarInterpreter())
  {
    vtkErrorMacro("No packet interpreter selected.");
  }

  // load the calibration file only now to allow to set it before the interpreter.
  if (this->GetLidarInterpreter()->GetCalibrationFileName() != this->CalibrationFileName)
  {
    this->GetLidarInterpreter()->SetCalibrationFileName(this->CalibrationFileName);
    this->GetLidarInterpreter()->LoadCalibration(this->CalibrationFileName);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkLidarStream::RequestData(vtkInformation* vtkNotUsed(request),
                                vtkInformationVector** vtkNotUsed(inputVector),
                                vtkInformationVector* outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  int numberOfFrameAvailable = 0;
  {
    std::lock_guard<std::mutex> lock(this->DataMutex);
    numberOfFrameAvailable = this->CheckForNewData();
    if (numberOfFrameAvailable != 0)
    {
      vtkSmartPointer<vtkPolyData> polyData = this->Frames.back();
      output->ShallowCopy(polyData);
      this->Frames.clear();
      this->LastFrameProcessed += numberOfFrameAvailable;
    }
  }

  if (this->DetectFrameDropping)
  {
    if (numberOfFrameAvailable > 1)
    {
      std::stringstream text;
      text << "WARNING : At frame " << std::right << std::setw(6) << this->LastFrameProcessed
           << " Drop " << std::right << std::setw(2) << numberOfFrameAvailable-1 << " frame(s)\n";
      vtkWarningMacro( << text.str() );
    }
  }

  vtkTable* calibration = vtkTable::GetData(outputVector,1);
  calibration->ShallowCopy(this->GetLidarInterpreter()->GetCalibrationTable());

  return 1;
}

//----------------------------------------------------------------------------
void vtkLidarStream::Start()
{
  this->Calibrate(); // Load calibration
  vtkStream::Start();
}

//----------------------------------------------------------------------------
void vtkLidarStream::AddNewData()
{
  vtkSmartPointer<vtkPolyData> LastFrame = this->GetLidarInterpreter()->GetLastFrameAvailable();
  this->Frames.push_back(LastFrame);

  // This prevents accumulating frames forever when "Pause" is toggled
  // There is little reason to use a std::deque to cache the frames, so
  // while waiting for a better fix, lets set a maximum size to the queue.
  // If this maximum size is too big (>= 100) this seems to cause a
  // memory leak.
  // TODO: investigate (not needed if a refactor removes the queue)
  // TODO: check this->Frames.size() before the push_back of the new frame
  //       to avoid adding then removing the same element
  if (this->Frames.size() > 2)
  {
    this->Frames.pop_back();
  }
}

//----------------------------------------------------------------------------
void vtkLidarStream::ClearAllDataAvailable()
{
  this->GetLidarInterpreter()->ClearAllFramesAvailable();
}

//----------------------------------------------------------------------------
int vtkLidarStream::CheckForNewData()
{
  return this->Frames.size();
}

//----------------------------------------------------------------------------
vtkLidarPacketInterpreter* vtkLidarStream::GetLidarInterpreter()
{
  return vtkLidarPacketInterpreter::SafeDownCast(this->Interpreter);
}

//----------------------------------------------------------------------------
void vtkLidarStream::SetLidarInterpreter(vtkLidarPacketInterpreter* interpreter)
{
  this->Interpreter = interpreter;
}
