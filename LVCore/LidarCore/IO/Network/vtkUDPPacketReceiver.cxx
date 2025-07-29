/*=========================================================================

  Program: LidarView
  Module:  vtkUDPPacketReceiver.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUDPPacketReceiver.h"

#include "vtkPacketRecorder.h"
#include "vtkSynchronizedQueue.h"
#include "vtkUDPReceiverSocketImpl.h"

#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include <tins/tins.h>

#include <boost/asio.hpp>

#include <atomic>
#include <ctime>
#include <memory>
#include <thread>

namespace
{
constexpr unsigned int PKT_CACHE_SIZE = 500;
constexpr unsigned int PROTOCOL_UDP = 17;

using QueueType = vtkSynchronizedQueue<vtkUDPReceiverSocketImpl::PacketType>;

//-----------------------------------------------------------------------------
void StopThread(std::unique_ptr<std::thread>& thread)
{
  if (thread)
  {
    if (thread->joinable())
    {
      thread->join();
    }
    thread.reset();
  }
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUDPPacketReceiver);

//-----------------------------------------------------------------------------
class vtkUDPPacketReceiver::vtkInternals
{
public:
  boost::asio::io_service IOService;
  std::vector<std::unique_ptr<vtkUDPReceiverSocketImpl>> ReceiverSockets;

  std::unique_ptr<QueueType> DataQueue;
  ConsumeCallback Callback;

  vtkPacketRecorder* Writer;

  //------------------------------------------------------------------------------
  bool OpenSocket(const vtkUDPReceiverSocketImpl::Parameters& params)
  {
    if (params.listeningPorts.empty())
    {
      vtkGenericWarningMacro("No port specified, cannot open sockets.");
      return false;
    }
    if (!params.forwardPorts.empty() && params.forwardPorts.size() != params.listeningPorts.size())
    {
      vtkGenericWarningMacro("The same number of listening ports and forward ports are expected.");
      return false;
    }
    this->DataQueue = std::make_unique<QueueType>(::PKT_CACHE_SIZE);

    for (unsigned int portIdx = 0; portIdx < params.listeningPorts.size(); portIdx++)
    {
      auto callback = [this](vtkUDPReceiverSocketImpl::PacketType& packet)
      { this->DataQueue->Enqueue(packet); };
      this->ReceiverSockets.emplace_back(
        std::make_unique<vtkUDPReceiverSocketImpl>(this->IOService, callback));

      bool isOpen = this->ReceiverSockets.back()->Open(params, portIdx);
      if (isOpen)
      {
        this->ReceiverSockets.back()->ReceiveNextPacket();
      }
    }

    return true;
  }

  //------------------------------------------------------------------------------
  void StartIOService()
  {
    if (this->IOService.stopped())
    {
      this->IOService.restart();
    }
    boost::system::error_code errCode;
    this->IOService.run(errCode);
    if (errCode)
    {
      vtkErrorWithObjectMacro(
        nullptr, << "An error occurred when running boost io service: " << errCode.message());
    }
  }

  //------------------------------------------------------------------------------
  void CloseSocket()
  {
    for (auto& socket : this->ReceiverSockets)
    {
      socket->Close();
    }
    this->ReceiverSockets.clear();
    if (this->DataQueue)
    {
      this->DataQueue->StopQueue();
    }
  }

  //------------------------------------------------------------------------------
  void ConsumeLoop()
  {
    vtkUDPReceiverSocketImpl::PacketType packet;
    while (this->DataQueue->Dequeue(packet))
    {
      double timestamp = packet.timestamp.tv_sec + packet.timestamp.tv_usec * 1e-6;
      this->Callback(packet.data, timestamp);

      if (this->Writer && this->Writer->GetIsRecording())
      {
        Tins::UDP udp(packet.dstPort, packet.srcPort);
        udp /= Tins::RawPDU(packet.data);
        std::unique_ptr<Tins::PDU> pdu = std::make_unique<Tins::EthernetII>();
        if (packet.isIPv4)
        {
          Tins::IP ip(packet.dstAddress, packet.srcAddress);
          ip.protocol(::PROTOCOL_UDP);
          *pdu /= ip / udp;
        }
        else
        {
          Tins::IPv6 ip(packet.dstAddress, packet.srcAddress);
          // Hop limit is equivalent to TTL in IPv4, set to 64 to avoid warnings in wireshark.
          // We could also retrieve the real current hop with boost.
          ip.hop_limit(64);
          ip.next_header(::PROTOCOL_UDP);
          *pdu /= ip / udp;
        }
        pdu->serialize();
        Tins::Packet pkt(*pdu, Tins::Timestamp(packet.timestamp));
        this->Writer->AddPacketToWritingQueue(&pkt);
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkUDPPacketReceiver::vtkUDPPacketReceiver()
  : Internals(new vtkUDPPacketReceiver::vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkUDPPacketReceiver::~vtkUDPPacketReceiver()
{
  this->StopListening();
}

//----------------------------------------------------------------------------
bool vtkUDPPacketReceiver::StartListening(const std::vector<unsigned int>& ports,
  const ConsumeCallback& callback)
{
  if (this->IsListening())
  {
    // Stop the stream, to handle succeeding call to Start()
    this->StopListening();
  }

  vtkUDPReceiverSocketImpl::Parameters params;
  params.listeningPorts = ports;
  params.listeningAddress = this->LocalListeningAddress;
  params.multicastAddress = this->MulticastAddress;
  if (this->IsForwarding && !this->ForwardedIpAddress.empty())
  {
    for (auto& port : ports)
    {
      params.forwardPorts.emplace_back(port + this->ForwardedPortOffset);
    }
    params.forwardAddress = this->ForwardedIpAddress;
  }

  auto& internals = *this->Internals;
  bool started = internals.OpenSocket(params);
  if (started)
  {
    internals.Callback = callback;
    this->ReceiverThread =
      std::make_unique<std::thread>([&internals] { internals.StartIOService(); });
    this->ConsumerThread = std::make_unique<std::thread>([&internals] { internals.ConsumeLoop(); });
  }
  return started;
}

//----------------------------------------------------------------------------
void vtkUDPPacketReceiver::StopListening()
{
  auto& internals = *this->Internals;
  internals.CloseSocket();
  ::StopThread(this->ReceiverThread);
  ::StopThread(this->ConsumerThread);
  internals.DataQueue.reset();
}

//-----------------------------------------------------------------------------
bool vtkUDPPacketReceiver::IsListening()
{
  return !!this->ReceiverThread;
}

//-----------------------------------------------------------------------------
void vtkUDPPacketReceiver::SetRecorder(vtkPacketRecorder* writer)
{
  this->Internals->Writer = writer;
}
