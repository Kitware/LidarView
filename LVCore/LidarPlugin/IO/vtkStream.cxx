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

#include <boost/filesystem.hpp>

#include "PacketConsumer.h"
#include "PacketFileWriter.h"
#include "PacketReceiver.h"
#include "vtkInterpreter.h"

namespace {
std::string GetCrashAnalysingFileName()
{
  std::string appDir;

  // the home directory path is contained in the HOME environment variable on UNIX systems
  if (getenv("HOME"))
  {
    appDir = getenv("HOME");
    appDir += "/";
    appDir += SOFTWARE_NAME;
    appDir += "/";
    appDir += "LastData";
  }
  else
  {
    // On Windows, it's a concatanation of 2 environment variables
    appDir = getenv("HOMEDRIVE");
    appDir += getenv("HOMEPATH");
    appDir += "\\";
    appDir += SOFTWARE_NAME;
    appDir += "\\";
    appDir += "LastData";
  }

  // Checking if the application directory exists in the home directory and create it otherwise
  boost::filesystem::path appDirPath(appDir.c_str());

  if (!boost::filesystem::is_directory(appDirPath))
  {
    boost::filesystem::create_directory(appDirPath);
  }
  return appDir;
}
}

//-----------------------------------------------------------------------------
vtkStream::vtkStream()
{
  // cannot use default constructor in header "vtkStream() = default" due to forward declaration of some class
}

//-----------------------------------------------------------------------------
vtkStream::~vtkStream()
{
  this->Stop();
}

//-----------------------------------------------------------------------------
template<class T>
void vtkStream::SetAttributeAndRestartIfRunning(T& attribute, const T& value) {
  if (attribute != value)
  {
    attribute = value;
    if (this->IsRunning())
    {
      this->Stop();
      this->Start();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkStream::SetOutputFile(const std::string &value)
{
  SetAttributeAndRestartIfRunning(this->OutputFileName, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetForwardedIpAddress(const std::string &value)
{
  SetAttributeAndRestartIfRunning(this->ForwardedIpAddress, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetListeningPort(int value)
{
  SetAttributeAndRestartIfRunning(this->ListeningPort, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetMulticastAddress(const std::string& value)
{
  SetAttributeAndRestartIfRunning(this->MulticastAddress, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetLocalListeningAddress(const std::string& value)
{
  // TODO may need to soft comparaison
  SetAttributeAndRestartIfRunning(this->LocalListeningAddress, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetForwardedPort(int value)
{
  SetAttributeAndRestartIfRunning(this->ForwardedPort, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetIsForwarding(bool value)
{
  SetAttributeAndRestartIfRunning(this->IsForwarding, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetIsCrashAnalysing(bool value)
{
  SetAttributeAndRestartIfRunning(this->IsCrashAnalysing, value);
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
    return;
  }
  // The order of creation matters, as the receiver thread will enqueue packets on the consumer thread

  // Create and start the ConsumerThread
  //************************************
  this->ConsumerThread = std::make_unique<PacketConsumer>(this);
  this->ConsumerThread->Start();

  // Create and start the ReceiverThread
  //**********************************
  this->ReceiverThread = std::make_unique<PacketReceiver>(this->ListeningPort,
                                                        std::bind(&vtkStream::EnqueuePacket, this, std::placeholders::_1),
                                                        this->MulticastAddress,
                                                        this->LocalListeningAddress);
  if (this->IsForwarding)
  {
    this->ReceiverThread->EnableForwarding(this->ForwardedPort, this->ForwardedIpAddress);
  }
  if (this->IsCrashAnalysing)
  {
    this->ReceiverThread->EnableCrashAnalysing(GetCrashAnalysingFileName(), 5000);
  }
  this->ReceiverThread->SetFakeManufacturerMACAddress(this->GetInterpreter()->GetManufacturerMACAddress());
  this->ReceiverThread->Start();
}

//----------------------------------------------------------------------------
void vtkStream::Stop()
{
  // destruct the thread is the inverse order of creation
  this->ReceiverThread->Stop();
  this->ConsumerThread->Stop();
  if (this->WriterThread)
  {
    this->WriterThread->Stop();
  }
}

//-----------------------------------------------------------------------------
void vtkStream::EnqueuePacket(NetworkPacket* packet)
{
  NetworkPacket* packet2 = nullptr;
  if (this->WriterThread)
  {
    packet2 = new NetworkPacket(*packet);
    this->WriterThread->Enqueue(packet2);
  }
  assert(this->Consumer && "The receiver thread should be started before the consumer one");
  this->ConsumerThread->Enqueue(packet);

}

//-----------------------------------------------------------------------------
bool vtkStream::IsRunning()
{
  return this->ReceiverThread && this->ConsumerThread;
}
