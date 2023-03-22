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
#include "PacketReceiver.h"

#include "NetworkPacket.h"
#include "SynchronizedQueue.h"

#include <vtkMath.h>

#include <functional>

//-----------------------------------------------------------------------------
PacketReceiver::PacketReceiver(int port,
                               std::function<void(NetworkPacket *)> callback,
                               std::string multicastAddress,
                               std::string localListeningAddress)
  : Port(port)
  , MulticastAddress(multicastAddress)
  , LocalListeningAddress(localListeningAddress)
  , PacketCounter(0)
  , Socket(IOService)
  , ForwardedSocket(IOService)
  , ReceiverCallback(callback)
{
  // Check that the provided multicast ipadress is valid
  // Multicast enforce the use of ipv4 protocol
  if(multicastAddress != "")
  {
    // Check the listening address
    boost::system::error_code errCode;
    boost::asio::ip::address listen_address = boost::asio::ip::address::from_string(LocalListeningAddress, errCode);
    if(errCode != boost::system::errc::success)
    {
      vtkGenericWarningMacro("Listen address is not valid, listening on all local ip addresses on v4");
      listen_address = boost::asio::ip::address_v4::any();
    }
    if (listen_address.is_v6())
    {
      vtkGenericWarningMacro("Multicast with ipv6 proctocol is not supported.");
    }
    else
    {
      // Open the socket
      this->Socket.open(boost::asio::ip::udp::v4());

      // Tell the OS we accept to re-use the port address for an other app
      this->Socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));

      // Connect to multicast
      boost::system::error_code errCode;
      boost::asio::ip::address multicast_address = boost::asio::ip::address::from_string(multicastAddress, errCode);
      if (multicast_address.is_v6())
      {
        vtkGenericWarningMacro("Multicast ip address must be an ipv4 address");
      }
      else if (errCode == boost::system::errc::success && multicast_address.is_multicast())
      {
        try
        {
#ifdef _MSC_VER
            // On Windows : Bind the socket to listen_address (specific or INADDR_ANY) and to the defined port
            this->Socket.bind(boost::asio::ip::udp::endpoint(listen_address.to_v4(), port));
#else
            // On Linux and MacOS : Bind the socket to defined multicast address and to the defined port
            this->Socket.bind(boost::asio::ip::udp::endpoint(multicast_address.to_v4(), port));
#endif

          // If bind on multicast : Work for listening address = to the one of the internal network, not the one from wifi
          boost::asio::ip::multicast::join_group option(multicast_address.to_v4(), listen_address.to_v4());
          this->Socket.set_option(option);

          vtkGenericWarningMacro("Listening on " << listen_address.to_string() << " local IP, with multicast group " << multicast_address.to_string() << " ONLY.");
        }
        catch(...)
        {
          vtkGenericWarningMacro("Error while setting listening address for multicast, please correct it or leave empty to listen on all local ip addresses");
        }
      }
      else
      {
        vtkGenericWarningMacro("Multicast ip address not valid, please correct it or leave empty to ignore");
      }
    }
  }
  else
  {
    // If there is no multicast adress, the protocol can be v4 or v6
    // it's determine by the local listening adress
    // Check the listening address
    boost::system::error_code errCode;
    boost::asio::ip::address listen_address = boost::asio::ip::address::from_string(LocalListeningAddress, errCode);
    if(errCode != boost::system::errc::success)
    {
      vtkGenericWarningMacro("Listen address is not valid, listening on all local ip addresses on v6");
      listen_address = boost::asio::ip::address_v6::any();
    }

    // Open the Socket with the right protocol
    if (listen_address.is_v4())
    {
      this->Socket.open(boost::asio::ip::udp::v4());
    }
    else if (listen_address.is_v6())
    {
      this->Socket.open(boost::asio::ip::udp::v6());
      if (listen_address == boost::asio::ip::address_v6::any())
      {
        this->Socket.set_option(boost::asio::ip::v6_only(false));
      }
    }
    // Tell the OS we accept to re-use the port address for an other app
    this->Socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));

    try
      {
        // If no multicast address specified (Listen on broadcast and unicast)
        // Bind the socket to listen address (specific or INADDR_ANY) and to the defined port
        this->Socket.bind(boost::asio::ip::udp::endpoint(listen_address, port));
      }
      catch(...)
      {
        vtkGenericWarningMacro("Error while setting listening address, please correct it or leave empty to ignore");
      }
  }
}

//-----------------------------------------------------------------------------
PacketReceiver::~PacketReceiver()
{
  this->Stop();
  this->Socket.close();
  if (this->isForwarding)
  {
    this->ForwardedSocket.close();
  }

  // Close and delete the logs files. So that,
  // if a log file is present in the next session
  // it means that the software has been closed
  // properly (potentially a crash)
  if (this->IsCrashAnalysing)
  {
    this->CrashAnalysis.CloseAnalyzer();
    this->CrashAnalysis.DeleteLogFiles();
  }
}
//-----------------------------------------------------------------------------
void PacketReceiver::EnableForwarding(int forwardport, const std::string& forwarddestinationIp)
{
  assert(this->Thread && "You cannot call this function while running, please call it just after constructing the object");
  this->isForwarding = false;
  // Check that the provided forwarding ipadress is valid
  boost::system::error_code errCode;
  boost::asio::ip::address ipAddressForwarding = boost::asio::ip::address::from_string(forwarddestinationIp, errCode);
  if(errCode == boost::system::errc::success)
  {
    this->ForwardEndpoint = boost::asio::ip::udp::endpoint(ipAddressForwarding, forwardport);
    this->ForwardedSocket.open(ForwardEndpoint.protocol());
    // toward the forwarded ip address and port
    this->ForwardedSocket.set_option(boost::asio::ip::multicast::enable_loopback(
                                  true)); // Allow to send the packet on the same machine
    this->isForwarding = true;
   }
  else
  {
      vtkGenericWarningMacro("Forward IP address not valid, packets won't be forwarded");
      this->isForwarding = false;
  }
}

//-----------------------------------------------------------------------------
void PacketReceiver::Start()
{
  std::cout << "Start" << std::endl;
  this->WaitForNextPacket();
  // Make the callback run in otherthread
  this->Thread = std::make_unique<std::thread>([&]{ this->IOService.run(); });
}

//-----------------------------------------------------------------------------
void PacketReceiver::Stop()
{
  std::cout << "Stop" << std::endl;
  // Instead off calling close, we would like to call cancel, as this enable to reusse the Socket.
  // But cancel can be be silently ignored by the system as mention in the documentation
  // https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_stream_socket/cancel/overload1.html
  this->Socket.close();
  if (this->Thread)
  {
    if (this->Thread->joinable())
    {
      this->Thread->join();
    }
  }
}

//-----------------------------------------------------------------------------
void PacketReceiver::EnableCrashAnalysing(std::string filenameCrashAnalysis_, unsigned int nbrPacketToStore_)
{
  assert(this->Thread && "You cannot call this function while running, please call it just after constructing the object");
  this->IsCrashAnalysing = true;

  // Opening crash analysis file
  if (this->IsCrashAnalysing)
  {
    this->CrashAnalysis.SetNbrPacketsToStore(nbrPacketToStore_);
    this->CrashAnalysis.SetFilename(filenameCrashAnalysis_);
    this->CrashAnalysis.ArchivePreviousLogIfExist();
  }
}

//-----------------------------------------------------------------------------
void PacketReceiver::SetFakeManufacturerMACAddress(uint64_t value)
{
  this->FakeManufacturerMACAddress = value;
}

//-----------------------------------------------------------------------------
void PacketReceiver::SocketCallback(
  const boost::system::error_code& error, std::size_t numberOfBytes)
{
  if (error) // in case the Socket cancel == boost::asio::error::operation_aborted)
  {
    return;
  }
  unsigned short ourPort = static_cast<unsigned short>(this->Port);
  // endpoint::port() is in host byte order
  unsigned short sourcePort = this->SenderEndpoint.port();
  // sourceIP has network endianess (so big endian).
  std::array<unsigned char, 4> sourceIP = {192, 168, 0, 200};
  if (this->SenderEndpoint.address().is_v4())
  {
    for (int i = 0; i < 4; i++)
    {
      // the array types are differents and to_bytes() handles endianess
      sourceIP[i] = this->SenderEndpoint.address().to_v4().to_bytes()[i];
    }
  }
  // TODO: IPV6 is recorded as fake ipv4 packet -> create BuildEthernetIP6UDP
  NetworkPacket* packet = NetworkPacket::BuildEthernetIP4UDP(this->RXBuffer,
                                                       numberOfBytes,
                                                       sourceIP,
                                                       sourcePort,
                                                       ourPort,
                                                       this->FakeManufacturerMACAddress);

  if (this->isForwarding)
  {
    ForwardedSocket.send_to(boost::asio::buffer(packet->GetPayloadData(), packet->GetPayloadSize()), ForwardEndpoint);
  }

  if (this->IsCrashAnalysing)
  {
    this->CrashAnalysis.AddPacket(*packet);
  }

  this->ReceiverCallback(packet);

  this->WaitForNextPacket();

  if ((++this->PacketCounter % 5000) == 0)
  {
    std::cout << "RECV packets: " << this->PacketCounter << " on " << this->Port << std::endl;
  }

}

//-----------------------------------------------------------------------------
void PacketReceiver::WaitForNextPacket()
{
  this->Socket.async_receive_from(boost::asio::buffer(this->RXBuffer, BUFFER_SIZE),
                                  this->SenderEndpoint,
                                  std::bind(&PacketReceiver::SocketCallback, this, std::placeholders::_1,
                                              std::placeholders::_2));
}

