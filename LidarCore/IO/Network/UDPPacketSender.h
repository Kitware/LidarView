/*=========================================================================

  Program: LidarView
  Module:  UDPPacketSender.h

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>

#include "lvIONetworkModule.h"

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <functional>
#include <string>

class vtkPacketFileHandler;

/**
 * This class is responsible to send all packets in a .pcap file once.
 * To send a pcap file multiples times, a new UDPPacketSender should be created each time.
 */
class LVIONETWORK_EXPORT UDPPacketSender
{
public:
  UDPPacketSender(std::string pcapfile,
    std::string destinationio = "127.0.0.1",
    int lidarport = 2368,
    int positionport = 8308);
  ~UDPPacketSender();

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
