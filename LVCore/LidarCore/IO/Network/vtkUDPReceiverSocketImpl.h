/*=========================================================================

  Program: LidarView
  Module:  vtkUDPReceiverSocketImpl.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUDPReceiverSocketImpl_h
#define vtkUDPReceiverSocketImpl_h

#include "vtkUDPPacketReceiver.h"

#include <boost/asio.hpp>

#include <array>
#include <ctime>
#include <functional>
#include <string>
#include <vector>

constexpr unsigned int BUFFER_SIZE = 34000;

class vtkUDPReceiverSocketImpl
{
public:
  struct Parameters
  {
    std::vector<unsigned int> listeningPorts;
    std::string listeningAddress;
    std::string multicastAddress;
    std::vector<unsigned int> forwardPorts;
    std::string forwardAddress;
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

  vtkUDPReceiverSocketImpl(boost::asio::io_context& ioContext, HandleReceiveCallback callback);

  bool Open(const vtkUDPReceiverSocketImpl::Parameters& params, uint16_t portIdx);
  void Close();

  void ReceiveNextPacket();

private:
  boost::asio::ip::udp::socket ReceiveSocket;
  boost::asio::ip::udp::endpoint SenderEndpoint;
  boost::asio::ip::udp::socket ForwardSocket;
  boost::asio::ip::udp::endpoint ForwardEndpoint;
  std::array<uint8_t, BUFFER_SIZE> Buffer;
  HandleReceiveCallback ReceiveCallback;
};

#endif
