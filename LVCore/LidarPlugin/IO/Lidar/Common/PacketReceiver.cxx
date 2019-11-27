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
#include "NetworkSource.h"
#include "NetworkPacket.h"

#include <vtkMath.h>

//-----------------------------------------------------------------------------
PacketReceiver::PacketReceiver(boost::asio::io_service &io, int port, int forwardport,
                               std::string forwarddestinationIp, bool isforwarding, NetworkSource *parent,
                               std::string multicastAddress, std::string LocalListeningAddress)
  : isForwarding(isforwarding)
  , Port(port)
  , PacketCounter(0)
  , Socket(io)
  , ForwardedSocket(io)
  , Parent(parent)
  , IsReceiving(true)
  , ShouldStop(false)
  , MulticastAddress(multicastAddress)
  , LocalListeningAddress(LocalListeningAddress)
{
  // Opening the socket with an UDP v4 protocol (work for v6 protocol too)
  this->Socket.open(boost::asio::ip::udp::v4());
  // Tell the OS we accept to re-use the port address for an other app
  this->Socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));

  // If a local specific address is specified, we try to listen to it
  // Otherwise, we listen to any address (INADDR_ANY)
  boost::asio::ip::address listen_address = boost::asio::ip::address_v4::any();
  if (LocalListeningAddress != "" && strcmp(LocalListeningAddress.c_str(), "0.0.0.0") != 0)
  {
    boost::system::error_code errCode;
    boost::asio::ip::address local_listen_address = boost::asio::ip::address::from_string(LocalListeningAddress, errCode);
    if(errCode == 0)
    {
      listen_address = local_listen_address;
    }
  }

  // Bind the socket to listen_address (specific or INADDR_ANY) and to the defined port
  this->Socket.bind(boost::asio::ip::udp::endpoint(listen_address, port));

  if(multicastAddress != "")
  {
    // Connect to multicast
    boost::asio::ip::address multicast_address = boost::asio::ip::address_v4::from_string(multicastAddress);
    if (multicast_address.is_multicast())
    {
      boost::asio::ip::multicast::join_group option(multicast_address);
      this->Socket.set_option(option);
    }
  }

  // Check that the provided ipadress is valid
  boost::system::error_code errCode;
  boost::asio::ip::address ipAddressForwarding = boost::asio::ip::address_v4::from_string(forwarddestinationIp, errCode);

  // If the ip address is not valid we replace it by a generic
  // 0.0.0.0 ip address. This is due to the application
  // crashes on windows if a not valid ip address is provided
  // with error message:
  // Qt has caught an exception from an event handler. This
  // is not supported in Qt.
  if (errCode)
  {
    ipAddressForwarding = boost::asio::ip::address_v4::from_string("0.0.0.0");
    if (this->isForwarding)
    {
      vtkGenericWarningMacro("Forward ip address not valid, packets won't be forwarded");
      this->isForwarding = false;
    }
  }

  this->ForwardEndpoint = boost::asio::ip::udp::endpoint(ipAddressForwarding, forwardport);
  this->ForwardedSocket.open(ForwardEndpoint.protocol()); // Opening the socket with an UDP v4 protocol
  // toward the forwarded ip address and port
  this->ForwardedSocket.set_option(boost::asio::ip::multicast::enable_loopback(
                                true)); // Allow to send the packet on the same machine
}

//-----------------------------------------------------------------------------
PacketReceiver::~PacketReceiver()
{
  this->Socket.cancel();
  this->ForwardedSocket.cancel();
  {
    boost::unique_lock<boost::mutex> guard(this->IsReceivingMtx);
    this->ShouldStop = true;
    while (this->IsReceiving)
    {
      this->IsReceivingCond.wait(guard);
    }
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
void PacketReceiver::StartReceive()
{
  {
    boost::lock_guard<boost::mutex> guard(this->IsReceivingMtx);
    this->IsReceiving = true;
  }

  // expecting exactly 1206 bytes, using a larger buffer so that if a
  // larger packet arrives unexpectedly we'll notice it.
  this->Socket.async_receive_from(boost::asio::buffer(this->RXBuffer, BUFFER_SIZE),
                                  this->SenderEndpoint,
                                  boost::bind(&PacketReceiver::SocketCallback, this, boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
}

//-----------------------------------------------------------------------------
void PacketReceiver::EnableCrashAnalysing(std::string filenameCrashAnalysis_, unsigned int nbrPacketToStore_, bool isCrashAnalysing_)
{
  this->IsCrashAnalysing = isCrashAnalysing_;

  // Opening crash analysis file
  if (this->IsCrashAnalysing)
  {
    this->CrashAnalysis.SetNbrPacketsToStore(nbrPacketToStore_);
    this->CrashAnalysis.SetFilename(filenameCrashAnalysis_);
    this->CrashAnalysis.ArchivePreviousLogIfExist();
  }
}

//-----------------------------------------------------------------------------
void PacketReceiver::SocketCallback(
  const boost::system::error_code& error, std::size_t numberOfBytes)
{
  if (error || this->ShouldStop)
  {
    // This is called on cancel
    // TODO: Check other error codes
    {
      boost::lock_guard<boost::mutex> guard(this->IsReceivingMtx);
      this->IsReceiving = false;
    }
    this->IsReceivingCond.notify_one();

    return;
  }

  unsigned short ourPort = static_cast<unsigned short>(this->Port);
  // endpoint::port() is in host byte order
  unsigned short sourcePort = this->SenderEndpoint.port();
  // sourceIP has network endianess (so big endian).
  unsigned char sourceIP[4] = {192, 168, 0, 200};
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
                                                       ourPort);

  // std::cout << this->Socket.remote_endpoint().address() << std::endl;

  // I looked at the struct sockaddr* inside the sender endpoint, but no
  // other data than ip source and port source is provided (which is normal,
  // we are working at the application level).

  if (this->isForwarding)
  {
    ForwardedSocket.send_to(boost::asio::buffer(packet->GetPayloadData(), packet->GetPayloadSize()), ForwardEndpoint);
  }

  if (this->IsCrashAnalysing)
  {
    this->CrashAnalysis.AddPacket(*packet);
  }

  this->Parent->QueuePackets(packet);

  this->StartReceive();

  if ((++this->PacketCounter % 5000) == 0)
  {
    std::cout << "RECV packets: " << this->PacketCounter << " on " << this->Port << std::endl;
  }
}

