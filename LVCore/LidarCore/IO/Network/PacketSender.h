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

#include <vtkSmartPointer.h>

#include "lvIONetworkModule.h"

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <functional>
#include <string>

class vtkPacketFileHandler;

/**
 * \class PacketSender
 * \brief This class is responsible to send all packets in a .pcap file once.
 *        To send a pcap file multiples times, a new PacketSender should be created each time.
 */

class LVIONETWORK_EXPORT PacketSender
{
public:
  PacketSender(std::string pcapfile,
    std::string destinationio = "127.0.0.1",
    int lidarport = 2368,
    int positionport = 8308);
  ~PacketSender();

  /**
   * @brief sendAllPackets
   * @param speed factor compare to the recording
   * @param display_frequency display some information information every n packets.
   * @param callback function executed after sending a packet
   * @return
   */
  bool sendAllPackets(double speed = 1.0,
    int display_frequency = 0,
    std::function<void()> callback = nullptr);

  /**
   * @brief pumpPacket
   * @return the timestamp of the packet send
   */
  double pumpPacket();

  /**
   * @copydoc Done
   */
  bool IsDone() const;

  /**
   * @brief PacketCount return the number of packet already send
   */
  size_t GetPacketCount() const;

private:
  boost::asio::ip::udp::socket* LIDARSocket;
  boost::asio::ip::udp::endpoint LIDAREndpoint;

  boost::asio::ip::udp::socket* PositionSocket;
  boost::asio::ip::udp::endpoint PositionEndpoint;

  boost::asio::io_service IOService;

  vtkSmartPointer<vtkPacketFileHandler> PacketReader;
  //! Indicate if the end of the pcap file has been reach
  bool Done;
  //! Number of packet already send
  size_t PacketCount;
};
