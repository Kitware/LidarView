/*=========================================================================

  Program: LidarView
  Module:  vtkTCPPacketSender.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTCPPacketSender.h"

#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include "vtkPacketFileHandler.h"

#include <boost/asio.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace
{
constexpr int MS_TO_S_FACTOR = 1e6;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTCPPacketSender);

//-----------------------------------------------------------------------------
class vtkTCPPacketSender::vtkInternals
{
public:
  boost::asio::io_service IOService;
  std::unique_ptr<boost::asio::ip::tcp::socket> Socket;

  bool IsDone = false;
  int PacketCount = 0;
};

//------------------------------------------------------------------------------
vtkTCPPacketSender::vtkTCPPacketSender()
  : Internals(new vtkTCPPacketSender::vtkInternals())
{
  this->PacketReader = vtkSmartPointer<vtkPacketFileHandler>::New();
}

//------------------------------------------------------------------------------
bool vtkTCPPacketSender::SendAllPackets(double speed)
{
  this->PacketReader->Open(this->PCAPFileName);
  if (!this->PacketReader->IsOpen())
  {
    vtkErrorMacro("Unable to open packet file!");
    return false;
  }
  auto& internals = *this->Internals;

  boost::system::error_code errCode;
  auto listenAddress = boost::asio::ip::address::from_string(this->Address, errCode);
  if (errCode != boost::system::errc::success || listenAddress.is_unspecified())
  {
    vtkGenericWarningMacro(<< "Listen address is not valid: " << errCode.message());
    return false;
  }
  vtkLog(INFO, << this->Address << " " << this->Port);
  boost::asio::ip::tcp::endpoint endpoint(listenAddress, this->Port);

  boost::asio::ip::tcp::acceptor acceptor(internals.IOService, endpoint);

  try
  {
    internals.Socket = std::make_unique<boost::asio::ip::tcp::socket>(internals.IOService);

    // boost::asio::ip::tcp::socket socket(internals.IOService); // (3)

    // Waiting for connection
    acceptor.accept(*internals.Socket);

    vtkLog(INFO, << "connected!");

    internals.IsDone = false;
    auto replayStartTime = std::chrono::steady_clock::now();

    double pcapStartTime = this->PumpPacket();

    while (!internals.IsDone)
    {
      double pcapCurrentTime = this->PumpPacket();
      double pcapmicroSecondsSinceStart = (pcapCurrentTime - pcapStartTime) * ::MS_TO_S_FACTOR;

      // time from the wall clock
      auto replayCurrentTime = std::chrono::steady_clock::now();
      int replaymicroSecondsSinceStart =
        std::chrono::duration_cast<std::chrono::microseconds>(replayCurrentTime - replayStartTime)
          .count();

      // check if the replay is to much in advance compared to the pcap time step
      // if this is the case, we sleep the thread until the pcap catch up the replay
      double timeDelay = pcapmicroSecondsSinceStart / speed - replaymicroSecondsSinceStart;

      if (timeDelay > 0)
      {
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(timeDelay)));
      }
      if (internals.PacketCount % 100)
      {
        vtkLog(INFO, << "Packet sent " << internals.PacketCount);
      }
    }
    vtkLog(INFO, << "Done sending...");
  }
  catch (std::exception& e)
  {
    vtkLog(WARNING, << "Caught Exception: " << e.what());
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
double vtkTCPPacketSender::PumpPacket()
{

  auto& internals = *this->Internals;
  if (internals.IsDone)
  {
    return std::numeric_limits<double>::max();
  }

  // Get the next packet
  if (!this->PacketReader->ReadNextPacket())
  {
    internals.IsDone = true;
    return this->PacketReader->GetTimestamp();
  }
  const std::vector<uint8_t>& payload = this->PacketReader->GetPayload();

  if (payload.size() != 512)
  {
    internals.Socket->send(boost::asio::buffer(payload));
    internals.PacketCount++;
  }
  return this->PacketReader->GetTimestamp();
}
