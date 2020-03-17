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

// LOCAL
#include "NetworkSource.h"
#include "vtkPacketFileWriter.h"
#include "PacketReceiver.h"
#include "PacketFileWriter.h"
#include "PacketConsumer.h"

#define PACKET_TO_STORE_CRASH_ANALYSIS 5000

//-----------------------------------------------------------------------------
NetworkSource::~NetworkSource()
{
  this->Stop();

  delete this->DummyWork;

  if (this->Thread)
  {
    this->Thread->join();
  }
}

//-----------------------------------------------------------------------------
void NetworkSource::QueuePackets(NetworkPacket* packet)
{
  NetworkPacket* packet2 = nullptr;
  if (this->Writer)
  {
    packet2 = new NetworkPacket(*packet);
  }

  if (this->Consumer)
  {
    this->Consumer->Enqueue(packet);
  }

  if (this->Writer)
  {
    this->Writer->Enqueue(packet2);
  }
}

//-----------------------------------------------------------------------------
void NetworkSource::Start()
{
  if (!this->Thread)
  {
    std::cout << "Start listen" << std::endl;
    this->Thread.reset(
      new boost::thread(boost::bind(&boost::asio::io_service::run, &this->IOService)));
  }

  // Create work
  this->PortReceiver = boost::shared_ptr<PacketReceiver>(new PacketReceiver(
    this->IOService, ListeningPort, ForwardedPort, ForwardedIpAddress, IsForwarding, this, MulticastAddress, LocalListeningAddress));


  if (this->IsCrashAnalysing)
  {
    std::string appDir;

    // the home directory path is contained in the HOME environment variable on UNIX systems
    if (getenv("HOME"))
    {
      appDir = getenv("HOME");
      appDir += "/";
      appDir += SOFTWARE_NAME;
      appDir += "/";
    }
    else
    {
      // On Windows, it's a concatanation of 2 environment variables
      appDir = getenv("HOMEDRIVE");
      appDir += getenv("HOMEPATH");
      appDir += "\\";
      appDir += SOFTWARE_NAME;
      appDir += "\\";
    }

    // Checking if the application directory exists in the home directory and create it otherwise
    boost::filesystem::path appDirPath(appDir.c_str());

    if (!boost::filesystem::is_directory(appDirPath))
    {
      boost::filesystem::create_directory(appDirPath);
    }

    this->PortReceiver->EnableCrashAnalysing(
      appDir + "LastData", PACKET_TO_STORE_CRASH_ANALYSIS, this->IsCrashAnalysing);
  }

  this->PortReceiver->StartReceive();

}

//-----------------------------------------------------------------------------
void NetworkSource::Stop()
{
  // Kill the receiver
  this->PortReceiver.reset();
}
