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

#include "vtkSynchronizedQueue.h"

#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include <tins/tins.h>

#include <boost/asio.hpp>

#include <chrono>
#include <ctime>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

namespace
{
constexpr unsigned int BUFFER_SIZE = 34000;
constexpr unsigned int PKT_CACHE_SIZE = 500;
constexpr unsigned int PROTOCOL_UDP = 17;

struct PacketType
{
  timeval timestamp;
  uint16_t srcPort;
  uint16_t dstPort;
  std::string srcAddress;
  std::string dstAddress;
  bool isIPv4;
  std::vector<uint8_t> data;
};

using QueueType = vtkSynchronizedQueue<PacketType>;

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
  boost::asio::ip::udp::socket ReceiveSocket;
  boost::asio::ip::udp::socket ForwardSocket;
  boost::asio::ip::udp::endpoint SenderEndpoint;
  boost::asio::ip::udp::endpoint ForwardEndpoint;

  uint8_t Buffer[::BUFFER_SIZE];
  std::unique_ptr<QueueType> DataQueue;
  ConsumeCallback Callback;

  std::unique_ptr<Tins::PacketWriter> Writer;
  std::atomic<bool> IsRecording = false;

  //------------------------------------------------------------------------------
  vtkInternals()
    : ReceiveSocket(IOService)
    , ForwardSocket(IOService)
  {
  }

  //------------------------------------------------------------------------------
  bool OpenSocket(const Parameters& params)
  {
    boost::system::error_code errCode;
    if (!params.forwardAddress.empty())
    {
      boost::asio::ip::address ipAddressForwarding =
        boost::asio::ip::address::from_string(params.forwardAddress, errCode);
      if (errCode == boost::system::errc::success)
      {
        this->ForwardEndpoint =
          boost::asio::ip::udp::endpoint(ipAddressForwarding, params.forwardPort);
        this->ForwardSocket.open(this->ForwardEndpoint.protocol());
        // Allow to send the packet on the same machine
        this->ForwardSocket.set_option(boost::asio::ip::multicast::enable_loopback(true));
      }
      else
      {
        vtkGenericWarningMacro("Forward IP address not valid, packets won't be forwarded");
      }
    }

    auto listenAddress = boost::asio::ip::address::from_string(params.listeningAddress, errCode);
    if (errCode != boost::system::errc::success)
    {
      vtkGenericWarningMacro(
        << "Listen address is not valid, listening on all local ip addresses on v6");
      listenAddress = boost::asio::ip::address_v6::any();
    }

    bool useMulticast = false;
    boost::asio::ip::address multicastAddress;
    if (!params.multicastAddress.empty())
    {
      multicastAddress = boost::asio::ip::address::from_string(params.multicastAddress, errCode);
      if (errCode == boost::system::errc::success && multicastAddress.is_multicast())
      {
        if (multicastAddress.is_v4() && listenAddress.is_v4())
        {
          useMulticast = true;
        }
        else
        {
          vtkGenericWarningMacro("Multicast ip address must be an ipv4 address");
        }
      }
      else
      {
        vtkGenericWarningMacro(
          "Multicast ip address not valid, please correct it or leave empty to ignore");
      }
    }

    if (listenAddress.is_v4())
    {
      this->ReceiveSocket.open(boost::asio::ip::udp::v4());
    }
    else if (listenAddress.is_v6())
    {
      this->ReceiveSocket.open(boost::asio::ip::udp::v6());
      if (listenAddress == boost::asio::ip::address_v6::any())
      {
        this->ReceiveSocket.set_option(boost::asio::ip::v6_only(false));
      }
    }
    // Tell the OS we accept to re-use the port address for an other app
    this->ReceiveSocket.set_option(boost::asio::ip::udp::socket::reuse_address(true));

    try
    {
      if (useMulticast)
      {
#ifdef _MSC_VER
        // On Windows : Bind the socket to listen_address (specific or INADDR_ANY) and to the
        // defined port
        this->ReceiveSocket.bind(
          boost::asio::ip::udp::endpoint(listenAddress.to_v4(), params.listeningPort));
#else
        // On Linux and MacOS : Bind the socket to defined multicast address and to the defined
        // port
        this->ReceiveSocket.bind(
          boost::asio::ip::udp::endpoint(multicastAddress.to_v4(), params.listeningPort));
#endif

        // If bind on multicast : Work for listening address = to the one of the internal
        // network, not the one from wifi
        boost::asio::ip::multicast::join_group option(
          multicastAddress.to_v4(), listenAddress.to_v4());
        this->ReceiveSocket.set_option(option);
      }
      else
      {
        this->ReceiveSocket.bind(
          boost::asio::ip::udp::endpoint(listenAddress, params.listeningPort));
      }
    }
    catch (...)
    {
      vtkGenericWarningMacro(<< "Error while opening socket!");
      return false;
    }
    this->DataQueue = std::make_unique<QueueType>(::PKT_CACHE_SIZE);

    this->ReceivingNextPacket();
    return true;
  }

  //------------------------------------------------------------------------------
  void CloseSocket()
  {
    if (this->ForwardSocket.is_open())
    {
      this->ForwardSocket.close();
    }
    if (this->ReceiveSocket.is_open())
    {
      this->ReceiveSocket.close();
    }
    if (this->DataQueue)
    {
      this->DataQueue->StopQueue();
    }
  }

  //------------------------------------------------------------------------------
  void ConsumeLoop()
  {
    PacketType packet;
    while (this->DataQueue->Dequeue(packet))
    {
      double timestamp = packet.timestamp.tv_sec + packet.timestamp.tv_usec * 1e-6;
      this->Callback(packet.data, timestamp);

      if (this->IsRecording)
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
        this->Writer->write(pkt);
      }
    }
  }

  //------------------------------------------------------------------------------
  void ReceivingNextPacket()
  {
    this->ReceiveSocket.async_receive_from(boost::asio::buffer(this->Buffer, ::BUFFER_SIZE),
      this->SenderEndpoint,
      // [this](const boost::system::error_code& error, std::size_t numberOfBytes)
      // { this->HandleReceive(error, numberOfBytes); });
      std::bind(&vtkInternals::HandleReceive, this, std::placeholders::_1, std::placeholders::_2));
  }

private:
  //------------------------------------------------------------------------------
  void HandleReceive(const boost::system::error_code& error, std::size_t numberOfBytes)
  {
    if (error)
    {
      return;
    }

    if (numberOfBytes > 0)
    {
      PacketType packet;

      // Get current time since epoch and save it into timeval struct
      auto duration = std::chrono::system_clock::now().time_since_epoch();
      packet.timestamp.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
      packet.timestamp.tv_usec =
        std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;

      // Get port & address information
      packet.srcPort = this->SenderEndpoint.port();
      packet.srcAddress = this->SenderEndpoint.address().to_string();
      auto localEndpoint = this->ReceiveSocket.local_endpoint();
      packet.dstPort = localEndpoint.port();
      packet.dstAddress = localEndpoint.address().to_string();
      packet.isIPv4 = this->SenderEndpoint.address().is_v4();

      // Get packet content
      packet.data.resize(numberOfBytes);
      std::copy(this->Buffer, this->Buffer + numberOfBytes, packet.data.begin());
      this->DataQueue->Enqueue(packet);

      if (this->ForwardSocket.is_open())
      {
        this->ForwardSocket.send_to(
          boost::asio::buffer(this->Buffer, numberOfBytes), this->ForwardEndpoint);
      }
    }

    if (this->ReceiveSocket.is_open())
    {
      this->ReceivingNextPacket();
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
bool vtkUDPPacketReceiver::StartListening(const Parameters& params, const ConsumeCallback& callback)
{
  auto& internals = *this->Internals;
  // Stop the stream, to handle succeeding call to Start()
  if (internals.ForwardSocket.is_open() || internals.ReceiveSocket.is_open())
  {
    return false;
  }
  this->StopListening();

  bool started = internals.OpenSocket(params);
  if (started)
  {
    internals.Callback = callback;
    this->ReceiverThread =
      std::make_unique<std::thread>([&internals] { internals.IOService.run(); });
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
  auto& internals = *this->Internals;
  return internals.ReceiveSocket.is_open() && this->ReceiverThread;
}

//-----------------------------------------------------------------------------
void vtkUDPPacketReceiver::StartRecording(const std::string& filename)
{
  this->Internals->Writer =
    std::make_unique<Tins::PacketWriter>(filename, Tins::DataLinkType<Tins::EthernetII>());
  this->Internals->IsRecording = true;
}

//-----------------------------------------------------------------------------
void vtkUDPPacketReceiver::StopRecording()
{
  this->Internals->IsRecording = false;
  this->Internals->Writer.reset();
}

//-----------------------------------------------------------------------------
bool vtkUDPPacketReceiver::IsRecording()
{
  return this->Internals->IsRecording;
}