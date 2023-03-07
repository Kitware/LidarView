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

#ifndef PACKETRECEIVER_H
#define PACKETRECEIVER_H

// LOCAL
#include "CrashAnalysing.h"

// BOOST
#include <boost/asio.hpp>

#include <thread>
#include <functional>

class NetworkSource;
template<typename T>
class SynchronizedQueue;

/*!< Size of the buffer used to store the data received */
#define BUFFER_SIZE 34000

/*!< Number of packed save when the option CrashAnalysing is set */
#define NBR_PACKETS_SAVED  1500

/**
 * \class PacketReceiver
 * \brief This classs is reponsbale for listening on a socket and each time a packet is received,
 * it will enqueue the packet on a specific Queue. Here it is used to setup an UDP multicast protocol.
 * The packet receiver can forward the received packets and/or store them in a output bin file
*/
class PacketReceiver
{
public:
  /**
   * @brief PacketReceiver
   * @param port The port address which will receive the packet
   * @param forwardport The port adress which will receive the forwarded packets
   * @param forwarddestinationIp The IP adress of the computer which will receive the forwarded packets
   */
  PacketReceiver(int port, std::function<void(NetworkPacket*)> callback, std::string multicastAdress = "", std::string localListeningAddress = "::");

  ~PacketReceiver();

  void EnableForwarding(int forwardport, const std::string& forwarddestinationIp);

  /**
   * @brief Start Start to handle packets received on the already open socket.
   *
   * @warning Cannot start again once you call Stop().
   */
  void Start();

  /**
   * @brief Stop Stop and close the Socket, so no more packets are received.
   *
   * @warning Cannot call start after calling stop, as the socket is close and not only cancel.
   */
  void Stop();

  /**
   * @brief EnableCrashAnalysing
   * @param filenameCrashAnalysis_ the name of the output file
   * @param nbrPacketToStore the number of packets to store
   */
  void EnableCrashAnalysing(std::string filenameCrashAnalysis_, unsigned int nbrPacketToStore_);

  void SetFakeManufacturerMACAddress(uint64_t);

private:
  //!
  //! \brief Callback call when a new packet is receive
  //! \param error is boost::asio::error::operation_aborted in case the socket cancel all asynchronous connection
  //! \param numberOfBytes
  //!
  void SocketCallback(const boost::system::error_code& error, std::size_t numberOfBytes);
  // see https://stackoverflow.com/a/44273900
  /**
   * @brief Wait for a packet to be receive by binding the callback function to the socket and specify the buffer used
   * to receive the data
   */
  void WaitForNextPacket();

  std::unique_ptr<std::thread> Thread;
  boost::asio::io_service IOService;

  /*!< Allow or not the forwarding of the packets */
  bool isForwarding = false;

  /*!< EndPoint of the sender, contains only sender ip, sender port */
  boost::asio::ip::udp::endpoint SenderEndpoint;

  /*!< EndPoint to forward to, contain information such as forward ip, forward port */
  boost::asio::ip::udp::endpoint ForwardEndpoint;
  
  /*!< Port address which will receive the packet */
  int Port;                

  /*!< Multicast address which will receive the packet */
  std::string MulticastAddress;

  /*!< The Listening address to receive packets in case of multiples interfaces */
  std::string LocalListeningAddress;
  
  /*!< Number of packets received */
  int PacketCounter;

  /*!< Socket : determines the protocol used and the address used for the reception of the packets */
  boost::asio::ip::udp::socket Socket;

  /*!< Socket : determines the protocol used and the address used for the reception of the forwarded packets */
  boost::asio::ip::udp::socket ForwardedSocket;

  /*!< Buffer which will saved the data that arrived via the network
       It must be larger than the largest packet expected to be receive*/
  unsigned char RXBuffer[BUFFER_SIZE];

  /*!< Flag indicating if we should stop the listening, @todo check if we shouldn't use an atomic here */
  bool ShouldStop = false;

  bool IsCrashAnalysing = false;
  CrashAnalysisWriter CrashAnalysis;
  /*!< Manufacturer MAC address to fake in the constructed NetworkPacket, as this information is lost by using boost::asio */
  uint64_t FakeManufacturerMACAddress = 0;

  std::function<void(NetworkPacket*)> ReceiverCallback;
};

#endif // PACKETRECEIVER_H
