/*=========================================================================

  Program: LidarView
  Module:  vtkUDPPacketReceiver.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUDPPacketReceiver_h
#define vtkUDPPacketReceiver_h

#include "lvIONetworkModule.h"
#include "vtkStreamPacketHandler.h"

#include <vtkSetGet.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

class vtkPacketRecorder;

/**
 * vtkUDPPacketReceiver is designed to listen on a specified port and address.
 * It opens a socket and forwards all received data to a provided callback function.
 * Additionally, it includes an option to record the received data into a pcap file.
 */
class LVIONETWORK_EXPORT vtkUDPPacketReceiver : public vtkStreamPacketHandler
{
public:
  static vtkUDPPacketReceiver* New();
  vtkTypeMacro(vtkUDPPacketReceiver, vtkStreamPacketHandler);

  ///@{
  /**
   * The Listening address in case of multiples interfaces
   */
  vtkGetMacro(LocalListeningAddress, std::string);
  vtkSetMacro(LocalListeningAddress, std::string);
  ///@}

  ///@{
  /**
   * The multicast address to receive packets
   */
  vtkGetMacro(MulticastAddress, std::string);
  vtkSetMacro(MulticastAddress, std::string);
  ///@}

  ///@{
  /**
   * Allowing the forwarding of the packets
   */
  vtkGetMacro(IsForwarding, bool);
  vtkSetMacro(IsForwarding, bool);
  ///@}

  ///@{
  /**
   * The ip to send forwarded packets
   */
  vtkGetMacro(ForwardedIpAddress, std::string);
  vtkSetMacro(ForwardedIpAddress, std::string);
  ///@}

  ///@{
  /**
   * The ip to send forwarded packets
   */
  vtkGetMacro(ForwardedPortOffset, unsigned int);
  vtkSetMacro(ForwardedPortOffset, unsigned int);
  ///@}

  ///@{
  /**
   * Start / stop listening to the UDP stream.
   * When listening, two threads are created: one for receiving and one for consuming data,
   * which then forwards the data to the callback function.
   */
  bool StartListening(const std::vector<unsigned int>& ports,
    const ConsumeCallback& callback) override;
  void StopListening() override;
  bool IsListening() override;
  ///@}

  /**
   * If set the packet receiver will forward received packets to the recorder
   */
  void SetRecorder(vtkPacketRecorder* writer) override;

protected:
  vtkUDPPacketReceiver();
  ~vtkUDPPacketReceiver() override;

private:
  vtkUDPPacketReceiver(const vtkUDPPacketReceiver&) = delete;
  void operator=(const vtkUDPPacketReceiver&) = delete;

  std::string MulticastAddress;
  std::string LocalListeningAddress = "::";
  bool IsForwarding = false;
  std::string ForwardedIpAddress = "127.0.0.1";
  unsigned int ForwardedPortOffset = 0;

  std::unique_ptr<std::thread> ReceiverThread;
  std::unique_ptr<std::thread> ConsumerThread;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
