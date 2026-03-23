/*=========================================================================

  Program: LidarView
  Module:  vtkTCPPacketReceiver.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTCPPacketReceiver.h"

#include "vtkPacketRecorder.h"
#include "vtkSynchronizedQueue.h"

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
constexpr unsigned int PROTOCOL_TCP = 6;

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

constexpr unsigned int BUFFER_SIZE = 34000;

//-----------------------------------------------------------------------------
class vtkTCPReceiverSocketImpl
{
public:
  struct Parameters
  {
    std::vector<unsigned int> listeningPorts;
    std::string listeningAddress;
  };

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
  using HandleReceiveCallback = std::function<void(PacketType&)>;

  vtkTCPReceiverSocketImpl(boost::asio::io_context& ioContext, HandleReceiveCallback callback)
    : ReceiveSocket(ioContext)
    , ReceiveCallback(callback)
  {
  }

  bool Open(const vtkTCPReceiverSocketImpl::Parameters& params, uint16_t portIdx)
  {
    boost::system::error_code errCode;
    auto listenAddress = boost::asio::ip::address::from_string(params.listeningAddress, errCode);
    if (errCode != boost::system::errc::success)
    {
      vtkGenericWarningMacro(<< "Listen address is not valid: " << errCode.message());
      return false;
    }
    if (listenAddress.is_unspecified())
    {
      return false;
    }

    uint16_t port = params.listeningPorts[portIdx];
    this->ReceiveSocket.connect(boost::asio::ip::tcp::endpoint(listenAddress, port), errCode);
    if (errCode != boost::system::errc::success)
    {
      vtkGenericWarningMacro(<< "Error while opening socket!");
    }
    // Tell the OS we accept to re-use the port address for an other app
    this->ReceiveSocket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    return true;
  }

  void Close() { this->ReceiveSocket.close(); }

  void ReceiveNextPacket()
  {

    auto receiveCallback = [this](const boost::system::error_code& error, std::size_t numberOfBytes)
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
        packet.timestamp.tv_sec =
          std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        packet.timestamp.tv_usec =
          std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;

        // Get port & address information
        if (this->ReceiveSocket.is_open())
        {
          auto remoteEndpoint = this->ReceiveSocket.remote_endpoint();
          packet.srcPort = remoteEndpoint.port();
          packet.srcAddress = remoteEndpoint.address().to_string();
          packet.isIPv4 = remoteEndpoint.address().is_v4();

          auto localEndpoint = this->ReceiveSocket.local_endpoint();
          packet.dstPort = localEndpoint.port();
          packet.dstAddress = localEndpoint.address().to_string();
        }

        // Get packet content
        packet.data.resize(numberOfBytes);
        std::copy(this->Buffer.begin(), this->Buffer.begin() + numberOfBytes, packet.data.begin());
        this->ReceiveCallback(packet);

        packetReceived++;
      }

      if (this->ReceiveSocket.is_open())
      {
        this->ReceiveNextPacket();
      }
    };

    this->ReceiveSocket.async_receive(boost::asio::buffer(this->Buffer), receiveCallback);
  }

private:
  boost::asio::ip::tcp::socket ReceiveSocket;
  std::array<uint8_t, BUFFER_SIZE> Buffer;
  HandleReceiveCallback ReceiveCallback;
  int packetReceived = 0;
};

using QueueType = vtkSynchronizedQueue<vtkTCPReceiverSocketImpl::PacketType>;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTCPPacketReceiver);

//-----------------------------------------------------------------------------
class vtkTCPPacketReceiver::vtkInternals
{
public:
  boost::asio::io_service IOService;
  std::vector<std::unique_ptr<vtkTCPReceiverSocketImpl>> ReceiverSockets;

  std::unique_ptr<QueueType> DataQueue;
  ConsumeCallback Callback;

  vtkPacketRecorder* Writer;

  //------------------------------------------------------------------------------
  bool OpenSocket(const vtkTCPReceiverSocketImpl::Parameters& params)
  {
    if (params.listeningPorts.empty())
    {
      vtkGenericWarningMacro("No port specified, cannot open sockets.");
      return false;
    }
    this->DataQueue = std::make_unique<QueueType>(::PKT_CACHE_SIZE);

    for (unsigned int portIdx = 0; portIdx < params.listeningPorts.size(); portIdx++)
    {
      auto callback = [this](vtkTCPReceiverSocketImpl::PacketType& packet)
      { this->DataQueue->Enqueue(packet); };
      this->ReceiverSockets.emplace_back(
        std::make_unique<vtkTCPReceiverSocketImpl>(this->IOService, callback));

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
    vtkTCPReceiverSocketImpl::PacketType packet;
    while (this->DataQueue->Dequeue(packet))
    {
      double timestamp = packet.timestamp.tv_sec + packet.timestamp.tv_usec * 1e-6;
      this->Callback(packet.data, timestamp);

      if (this->Writer && this->Writer->GetIsRecording())
      {
        Tins::TCP tcp(packet.dstPort, packet.srcPort);
        tcp /= Tins::RawPDU(packet.data);
        std::unique_ptr<Tins::PDU> pdu = std::make_unique<Tins::EthernetII>();
        if (packet.isIPv4)
        {
          Tins::IP ip(packet.dstAddress, packet.srcAddress);
          ip.protocol(::PROTOCOL_TCP);
          *pdu /= ip / tcp;
        }
        else
        {
          Tins::IPv6 ip(packet.dstAddress, packet.srcAddress);
          // Hop limit is equivalent to TTL in IPv4, set to 64 to avoid warnings in wireshark.
          // We could also retrieve the real current hop with boost.
          ip.hop_limit(64);
          ip.next_header(::PROTOCOL_TCP);
          *pdu /= ip / tcp;
        }
        pdu->serialize();
        Tins::Packet pkt(*pdu, Tins::Timestamp(packet.timestamp));
        this->Writer->AddPacketToWritingQueue(&pkt);
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkTCPPacketReceiver::vtkTCPPacketReceiver()
  : Internals(new vtkTCPPacketReceiver::vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkTCPPacketReceiver::~vtkTCPPacketReceiver()
{
  this->StopListening();
}

//----------------------------------------------------------------------------
bool vtkTCPPacketReceiver::StartListening(const std::vector<unsigned int>& ports,
  const ConsumeCallback& callback)
{
  if (this->IsListening())
  {
    // Stop the stream, to handle succeeding call to Start()
    this->StopListening();
  }

  vtkTCPReceiverSocketImpl::Parameters params;
  params.listeningPorts = ports;
  params.listeningAddress = this->LocalListeningAddress;

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
void vtkTCPPacketReceiver::StopListening()
{
  auto& internals = *this->Internals;
  internals.CloseSocket();
  ::StopThread(this->ReceiverThread);
  ::StopThread(this->ConsumerThread);
  internals.DataQueue.reset();
}

//-----------------------------------------------------------------------------
bool vtkTCPPacketReceiver::IsListening()
{
  return !!this->ReceiverThread;
}

//-----------------------------------------------------------------------------
void vtkTCPPacketReceiver::SetRecorder(vtkPacketRecorder* writer)
{
  this->Internals->Writer = writer;
}
