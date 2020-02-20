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

#include "vtkStream.h"

#include <sstream>

#include "NetworkSource.h"
#include "PacketConsumer.h"
#include "PacketFileWriter.h"

//-----------------------------------------------------------------------------
vtkStream::vtkStream()
{
  this->Consumer = std::make_shared<PacketConsumer>();
  this->Writer = std::make_shared<PacketFileWriter>();
  this->Network = std::make_unique<NetworkSource>(this->Consumer, 2368, 2369, "127.0.0.1", false, false);
}

//-----------------------------------------------------------------------------
vtkStream::~vtkStream()
{
  this->Stop();
}

//-----------------------------------------------------------------------------
std::string vtkStream::GetOutputFile()
{
  return this->OutputFileName;
}

//-----------------------------------------------------------------------------
void vtkStream::SetOutputFile(const std::string &filename)
{
  this->OutputFileName  = filename;
}

//-----------------------------------------------------------------------------
std::string vtkStream::GetForwardedIpAddress()
{
  return this->Network->ForwardedIpAddress;
}

//-----------------------------------------------------------------------------
void vtkStream::SetForwardedIpAddress(const std::string &ipAddress)
{
  this->Network->ForwardedIpAddress = ipAddress;
}

//-----------------------------------------------------------------------------
template<class T>
void vtkStream::SetAttributeAndRestartIfRunning(T& attribute, const T& value) {
  bool wasRunning = this->Network != nullptr
      && this->Network->Thread != nullptr && this->Network->Thread->joinable()
      && this->Network->LidarPortReceiver != nullptr;
  this->Stop();
  attribute = value;
  if (wasRunning)
  {
    this->Start();
  }
}

//-----------------------------------------------------------------------------
int vtkStream::GetLidarPort()
{
  return this->Network->LidarPort;
}

//-----------------------------------------------------------------------------
void vtkStream::SetLidarPort(int value)
{
  if (this->Network->LidarPort != value)
  {
    SetAttributeAndRestartIfRunning(this->Network->LidarPort, value);
  }
}

//-----------------------------------------------------------------------------
std::string vtkStream::GetMulticastAddress()
{
  return this->Network->MulticastAddress;
}

//-----------------------------------------------------------------------------
void vtkStream::SetMulticastAddress(const std::string& value)
{
  if (this->Network->MulticastAddress != value)
  {
    SetAttributeAndRestartIfRunning(this->Network->MulticastAddress, value);
  }
}

//-----------------------------------------------------------------------------
std::string vtkStream::GetLocalListeningAddress()
{
  return this->Network->LocalListeningAddress;
}

//-----------------------------------------------------------------------------
void vtkStream::SetLocalListeningAddress(const std::string& value)
{
  if (strcmp(this->Network->LocalListeningAddress.c_str(), value.c_str()) != 0)
  {
    SetAttributeAndRestartIfRunning(this->Network->LocalListeningAddress, value);
  }
}

//-----------------------------------------------------------------------------
int vtkStream::GetGPSPort()
{
  return this->Network->GPSPort;
}

//-----------------------------------------------------------------------------
void vtkStream::SetGPSPort(int value)
{
  this->Network->GPSPort = value;
}

//-----------------------------------------------------------------------------
int vtkStream::GetForwardedLidarPort()
{
  return this->Network->ForwardedLidarPort;
}

//-----------------------------------------------------------------------------
void vtkStream::SetForwardedLidarPort(int value)
{
  this->Network->ForwardedLidarPort = value;
}

//-----------------------------------------------------------------------------
int vtkStream::GetForwardedGPSPort()
{
  return this->Network->ForwardedGPSPort;
}

//-----------------------------------------------------------------------------
void vtkStream::SetForwardedGPSPort(int value)
{
  this->Network->ForwardedGPSPort = value;
}

//-----------------------------------------------------------------------------
bool vtkStream::GetIsForwarding()
{
  return this->Network->IsForwarding;
}

//-----------------------------------------------------------------------------
void vtkStream::EnableGPSListening(bool value)
{
  this->Network->ListenGPS = value;
}

//-----------------------------------------------------------------------------
void vtkStream::SetIsForwarding(bool value)
{
  this->Network->IsForwarding = value;
}

//-----------------------------------------------------------------------------
bool vtkStream::GetIsCrashAnalysing()
{
  return this->Network->IsCrashAnalysing;
}

//-----------------------------------------------------------------------------
void vtkStream::SetIsCrashAnalysing(bool value)
{
  this->Network->IsCrashAnalysing = value;
}

//-----------------------------------------------------------------------------
bool vtkStream::GetNeedsUpdate()
{
  if (this->CheckForNewData())
  {
    this->Modified();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkStream::Start()
{
  if (!this->GetInterpreter())
  {
    vtkErrorMacro("no interpreter is set")
  }
  this->Consumer->SetInterpreter(this->GetInterpreter());

  this->Consumer->SetStream(this);

  if (this->OutputFileName.length())
  {
    this->Writer->Start(this->OutputFileName);
  }

  this->Network->Writer.reset();

  if (this->Writer->IsOpen())
  {
    this->Network->Writer = this->Writer;
  }

  this->Consumer->Start();

  this->Network->Start();
}

//----------------------------------------------------------------------------
void vtkStream::Stop()
{
  this->Network->Stop();
  this->Consumer->Stop();
  this->Writer->Stop();
}
