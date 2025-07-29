/*=========================================================================

  Program: LidarView
  Module:  vtkUDPReceiverSocketImpl.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUDPReceiverSocketImpl.h"

#include <vtkObject.h>
#include <vtkSetGet.h>

#include <chrono>

//-----------------------------------------------------------------------------
vtkUDPReceiverSocketImpl::vtkUDPReceiverSocketImpl(boost::asio::io_context& ioContext,
  HandleReceiveCallback callback)
  : ReceiveSocket(ioContext)
  , ForwardSocket(ioContext)
  , ReceiveCallback(callback)
{
}

//-----------------------------------------------------------------------------
bool vtkUDPReceiverSocketImpl::Open(const vtkUDPReceiverSocketImpl::Parameters& params,
  uint16_t portIdx)
{
  boost::system::error_code errCode;
  if (!params.forwardAddress.empty() && portIdx < params.forwardPorts.size())
  {
    boost::asio::ip::address ipAddressForwarding =
      boost::asio::ip::address::from_string(params.forwardAddress, errCode);
    if (errCode == boost::system::errc::success)
    {
      // If more than one port ignore specified forward port and forward on the same port.
      uint16_t forwardPort = params.forwardPorts[portIdx];
      this->ForwardEndpoint = boost::asio::ip::udp::endpoint(ipAddressForwarding, forwardPort);
      this->ForwardSocket.open(this->ForwardEndpoint.protocol(), errCode);
      if (!errCode)
      {
        // Allow to send the packet on the same machine
        this->ForwardSocket.set_option(boost::asio::ip::multicast::enable_loopback(true));
      }
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
    this->ReceiveSocket.open(boost::asio::ip::udp::v4(), errCode);
  }
  else if (listenAddress.is_v6())
  {
    this->ReceiveSocket.open(boost::asio::ip::udp::v6(), errCode);
    if (listenAddress == boost::asio::ip::address_v6::any() &&
      errCode == boost::system::errc::success)
    {
      this->ReceiveSocket.set_option(boost::asio::ip::v6_only(false));
    }
  }
  if (errCode != boost::system::errc::success)
  {
    vtkErrorWithObjectMacro(nullptr, "Could not open the receive socket!");
    return false;
  }

  try
  {
    // Tell the OS we accept to re-use the port address for an other app
    this->ReceiveSocket.set_option(boost::asio::ip::udp::socket::reuse_address(true));

    uint16_t port = params.listeningPorts[portIdx];
    if (useMulticast)
    {
#ifdef _MSC_VER
      // On Windows : Bind the socket to listen_address (specific or INADDR_ANY) and to the
      // defined port
      this->ReceiveSocket.bind(boost::asio::ip::udp::endpoint(listenAddress.to_v4(), port));
#else
      // On Linux and MacOS : Bind the socket to defined multicast address and to the defined
      // port
      this->ReceiveSocket.bind(boost::asio::ip::udp::endpoint(multicastAddress.to_v4(), port));
#endif

      // If bind on multicast : Work for listening address = to the one of the internal
      // network, not the one from wifi
      boost::asio::ip::multicast::join_group option(
        multicastAddress.to_v4(), listenAddress.to_v4());
      this->ReceiveSocket.set_option(option);
    }
    else
    {
      this->ReceiveSocket.bind(boost::asio::ip::udp::endpoint(listenAddress, port));
    }
  }
  catch (...)
  {
    vtkGenericWarningMacro(<< "Error while opening socket!");
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkUDPReceiverSocketImpl::Close()
{
  try
  {
    if (this->ForwardSocket.is_open())
    {
      this->ForwardSocket.close();
    }
    if (this->ReceiveSocket.is_open())
    {
      this->ReceiveSocket.close();
    }
  }
  catch (...)
  {
  }
  this->ReceiveCallback = nullptr;
}

//-----------------------------------------------------------------------------
void vtkUDPReceiverSocketImpl::ReceiveNextPacket()
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
      packet.timestamp.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
      packet.timestamp.tv_usec =
        std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;

      // Get port & address information
      packet.srcPort = this->SenderEndpoint.port();
      packet.srcAddress = this->SenderEndpoint.address().to_string();
      if (this->ReceiveSocket.is_open())
      {
        boost::system::error_code errCode;
        auto localEndpoint = this->ReceiveSocket.local_endpoint(errCode);
        if (errCode == boost::system::errc::success)
        {
          packet.dstPort = localEndpoint.port();
          packet.dstAddress = localEndpoint.address().to_string();
        }
      }
      packet.isIPv4 = this->SenderEndpoint.address().is_v4();

      // Get packet content
      packet.data.resize(numberOfBytes);
      std::copy(this->Buffer.begin(), this->Buffer.begin() + numberOfBytes, packet.data.begin());
      if (this->ReceiveCallback)
      {
        this->ReceiveCallback(packet);
      }

      if (this->ForwardSocket.is_open())
      {
        boost::system::error_code errCode;
        this->ForwardSocket.send_to(
          boost::asio::buffer(packet.data), this->ForwardEndpoint, 0, errCode);
      }
    }

    if (this->ReceiveSocket.is_open())
    {
      this->ReceiveNextPacket();
    }
  };
  if (this->ReceiveSocket.is_open() && this->ReceiveCallback)
  {
    this->ReceiveSocket.async_receive_from(
      boost::asio::buffer(this->Buffer), this->SenderEndpoint, receiveCallback);
  }
}