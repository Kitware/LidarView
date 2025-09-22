/*=========================================================================

  Program: LidarView
  Module:  vtkTCPPacketReceiver.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkTCPPacketReceiver_h
#define vtkTCPPacketReceiver_h

#include "lvIONetworkModule.h"
#include "vtkStreamPacketHandler.h"

#include <vtkSetGet.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

class vtkPacketRecorder;

/**
 * vtkTCPPacketReceiver is designed to listen on a specified port and address.
 * It opens a socket and forwards all received data to a provided callback function.
 * Additionally, it includes an option to record the received data into a pcap file.
 */
class LVIONETWORK_EXPORT vtkTCPPacketReceiver : public vtkStreamPacketHandler
{
public:
  vtkTypeMacro(vtkTCPPacketReceiver, vtkStreamPacketHandler);
  static vtkTCPPacketReceiver* New();

  ///@{
  /**
   * The Listening address in case of multiples interfaces
   */
  vtkGetMacro(LocalListeningAddress, std::string);
  vtkSetMacro(LocalListeningAddress, std::string);
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
  vtkTCPPacketReceiver();
  ~vtkTCPPacketReceiver() override;

private:
  vtkTCPPacketReceiver(const vtkTCPPacketReceiver&) = delete;
  void operator=(const vtkTCPPacketReceiver&) = delete;

  std::string LocalListeningAddress = "::";

  std::unique_ptr<std::thread> ReceiverThread;
  std::unique_ptr<std::thread> ConsumerThread;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
