/*=========================================================================

  Program: LidarView
  Module:  UDPPacketSender.cxx

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "UDPPacketSender.h"
#include "vtkPacketFileHandler.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

//-----------------------------------------------------------------------------
UDPPacketSender::UDPPacketSender(std::string pcapfile,
  std::string destinationIp,
  int lidarPort,
  int positionPort)
  : LIDARSocket(nullptr)
  , LIDAREndpoint(boost::asio::ip::address_v4::from_string(destinationIp), lidarPort)
  , PositionSocket(nullptr)
  , PositionEndpoint(boost::asio::ip::address_v4::from_string(destinationIp), positionPort)
  , Done(false)
  , PacketCount(0)
{
  this->PacketReader = vtkSmartPointer<vtkPacketFileHandler>::New();
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
UDPPacketSender::~UDPPacketSender()
{
  delete this->LIDARSocket;
  delete this->PositionSocket;
}

//-----------------------------------------------------------------------------
bool UDPPacketSender::sendAllPackets(double speed,
  int display_frequency,
  std::function<void()> callback)
{
  const int OUTPUT_WIDTH = 15; // width of the column (#packet, duration, ...) in the output stream
  const int microSecondsPerSecond = 1e6;

  try
  {
    auto replayStartTime = std::chrono::steady_clock::now();
    if (display_frequency > 0)
    {
      // output the column header for the displayed values
      std::cout << "----------------------------------------------------------------------------"
                << std::endl
                << std::right << std::setw(OUTPUT_WIDTH) << "# packets" << std::right
                << std::setw(OUTPUT_WIDTH) << "duration (s)" << std::right
                << std::setw(OUTPUT_WIDTH) << "f (Hz)" << std::right << std::setw(OUTPUT_WIDTH)
                << "delay (us)" << std::endl
                << "----------------------------------------------------------------------------"
                << std::endl;
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
        std::chrono::duration_cast<std::chrono::microseconds>(replayCurrentTime - replayStartTime)
          .count();

      // check if the replay is to much in advance compared to the pcap time step
      // if this is the case, we sleep the thread until the pcap catch up the replay
      double time_delay =
        static_cast<double>(pcapmicroSecondsSinceStart) / speed - replaymicroSecondsSinceStart;

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
        double secondSinceStart =
          static_cast<double>(replaymicroSecondsSinceStart) / microSecondsPerSecond;

        // Nice output
        std::cout << std::fixed << std::right << std::setw(OUTPUT_WIDTH) << nbPacketSended
                  << std::fixed << std::right << std::setw(OUTPUT_WIDTH) << secondSinceStart
                  << std::right << std::setw(OUTPUT_WIDTH)
                  << static_cast<double>(nbPacketSended) / secondSinceStart << std::right
                  << std::setw(OUTPUT_WIDTH) << time_delay << std::endl;
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
double UDPPacketSender::pumpPacket()
{
  if (this->Done)
  {
    return std::numeric_limits<int>::max();
  }

  // Get the next packet
  if (!this->PacketReader->ReadNextPacket())
  {
    this->Done = true;
    return this->PacketReader->GetTimestamp();
  }
  const std::vector<uint8_t>& payload = this->PacketReader->GetPayload();

  // Position packet
  if (payload.size() == 512)
  {
    this->PositionSocket->send_to(boost::asio::buffer(payload), this->PositionEndpoint);
  }
  else // Lidar packet
  {
    this->LIDARSocket->send_to(boost::asio::buffer(payload), this->LIDAREndpoint);
  }
  this->PacketCount++;
  return this->PacketReader->GetTimestamp();
}

//-----------------------------------------------------------------------------
size_t UDPPacketSender::GetPacketCount() const
{
  return this->PacketCount;
}

//-----------------------------------------------------------------------------
bool UDPPacketSender::IsDone() const
{
  return this->Done;
}
