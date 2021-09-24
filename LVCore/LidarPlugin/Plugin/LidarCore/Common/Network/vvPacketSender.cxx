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

#include "vvPacketSender.h"
#include "Common/Network/vtkPacketFileReader.h"

#include <chrono>
#include <thread>

//-----------------------------------------------------------------------------
vvPacketSender::vvPacketSender(
  std::string pcapfile, std::string destinationIp, int lidarPort, int positionPort)
  : LIDARSocket(0)
  , LIDAREndpoint(boost::asio::ip::address_v4::from_string(destinationIp), lidarPort)
  , PositionSocket(0)
  , PositionEndpoint(boost::asio::ip::address_v4::from_string(destinationIp), positionPort)
  , PacketReader(0)
  , Done(false)
  , PacketCount(0)
{
  this->PacketReader = new vtkPacketFileReader;
  this->PacketReader->Open(pcapfile);
  if (!this->PacketReader->IsOpen())
  {
    throw std::runtime_error("Unable to open packet file");
  }

  this->LIDARSocket = new boost::asio::ip::udp::socket(this->IOService);
  this->LIDARSocket->open(this->LIDAREndpoint.protocol());
  this->LIDARSocket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
  // Allow to send the packet on the same machine
  this->LIDARSocket->set_option(boost::asio::ip::multicast::enable_loopback(true));

  this->PositionSocket = new boost::asio::ip::udp::socket(this->IOService);
  this->PositionSocket->open(this->PositionEndpoint.protocol());
  this->PositionSocket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
  // Allow to send the packet on the same machine
  this->PositionSocket->set_option(boost::asio::ip::multicast::enable_loopback(true));
}

//-----------------------------------------------------------------------------
vvPacketSender::~vvPacketSender()
{
  delete this->LIDARSocket;
  delete this->PositionSocket;
}

//-----------------------------------------------------------------------------
bool vvPacketSender::sendAllPackets(double speed, int display_frequency, std::function<void()> callback)
{
  const int OUTPUT_WIDTH = 15; // width of the column (#packet, duration, ...) in the output stream
  const int microSecondsPerSecond = 1e6;

  try
  {
    auto replayStartTime = std::chrono::steady_clock::now();
    if (display_frequency > 0)
    {
    // output the column header for the displayed values
    std::cout << "----------------------------------------------------------------------------" << std::endl
              << std::right << std::setw(OUTPUT_WIDTH) << "# packets"
              << std::right << std::setw(OUTPUT_WIDTH) << "duration (s)"
              << std::right << std::setw(OUTPUT_WIDTH) << "f (Hz)"
              << std::right << std::setw(OUTPUT_WIDTH) << "delay (us)"
              << std::endl
              << "----------------------------------------------------------------------------" << std::endl;
    }

    // Case starting time
    double pcapStartTime = this->pumpPacket();

    while (!this->IsDone())
    {
      // the callback function is call for each packet
      if (callback)
      {
        callback();
      }
      // time from the pcap file
      double pcapCurrentTime = this->pumpPacket();
      double pcapmicroSecondsSinceStart = (pcapCurrentTime - pcapStartTime) * microSecondsPerSecond;

      // time from the wall clock
      auto replayCurrentTime = std::chrono::steady_clock::now();
      int replaymicroSecondsSinceStart =
          std::chrono::duration_cast<std::chrono::microseconds>(replayCurrentTime - replayStartTime).count();

      // check if the replay is to much in advance compared to the pcap time step
      // if this is the case, we sleep the thread until the pcap catch up the replay
      double time_delay = static_cast<double>(pcapmicroSecondsSinceStart) / speed  - replaymicroSecondsSinceStart;

      if (time_delay > 0)
      {
      std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(time_delay)));
      }

      // [HACK start] slowdown sending rate when using the loopback interface as
      // it's a much faster interface than any real one.
      // on windows, it's not possible to sleep for a few microseconds only
      // so we do it just on unix system
      #if defined(unix) || defined(__unix__) || defined(__unix)
      if (LIDAREndpoint.address() == boost::asio::ip::address::from_string("127.0.0.1"))
      {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
      #endif
      // [HACK end]

      // Display the user some information
      if (display_frequency > 0 && (this->GetPacketCount() % display_frequency) == 0)
      {
        // Compute nb of packets sended including the one from previous loops
        int nbPacketSended = this->GetPacketCount();

        // Compute time since the replay began
        double secondSinceStart = static_cast<double>(replaymicroSecondsSinceStart) / microSecondsPerSecond;

        // Nice output
        std::cout << std::fixed
                  << std::right << std::setw(OUTPUT_WIDTH) << nbPacketSended
                  << std::fixed << std::right << std::setw(OUTPUT_WIDTH) << secondSinceStart
                  << std::right << std::setw(OUTPUT_WIDTH)
                  << static_cast<double>(nbPacketSended) /  secondSinceStart
                  << std::right << std::setw(OUTPUT_WIDTH) << time_delay
                  << std::endl;
      }
    }
  }
  catch (std::exception& e)
  {
    std::cout << "Caught Exception: " << e.what() << std::endl;
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
double vvPacketSender::pumpPacket()
{
  if (this->Done)
  {
    return std::numeric_limits<int>::max();
  }

  // some return value
  const unsigned char* data = 0;
  unsigned int dataLength = 0;
  double timeSinceStart = 0;

  // Get the next packet
  if (!this->PacketReader->NextPacket(data, dataLength, timeSinceStart))
  {
    this->Done = true;
    return timeSinceStart;
  }

  // Position packet
  if ((dataLength == 512))
  {
    this->PositionSocket->send_to(
      boost::asio::buffer(data, dataLength), this->PositionEndpoint);
  }
  else // Lidar packet
  {
    this->LIDARSocket->send_to(
      boost::asio::buffer(data, dataLength), this->LIDAREndpoint);
  }
  this->PacketCount++;
  return timeSinceStart;
}

//-----------------------------------------------------------------------------
size_t vvPacketSender::GetPacketCount() const
{
  return this->PacketCount;
}

//-----------------------------------------------------------------------------
bool vvPacketSender::IsDone() const
{
  return this->Done;
}
