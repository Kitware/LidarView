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
#include <vtkObject.h>
#include <vtkSetGet.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

/**
 * vtkUDPPacketReceiver is designed to listen on a specified port and address.
 * It opens a socket and forwards all received data to a provided callback function.
 * Additionally, it includes an option to record the received data into a pcap file.
 */
class LVIONETWORK_EXPORT vtkUDPPacketReceiver : public vtkObject
{
public:
  static vtkUDPPacketReceiver* New();
  vtkTypeMacro(vtkUDPPacketReceiver, vtkObject);

  struct Parameters
  {
    std::vector<uint16_t> listeningPorts;
    std::string listeningAddress;
    std::string multicastAddress;
    std::vector<uint16_t> forwardPorts;
    std::string forwardAddress;
  };
  using ConsumeCallback = std::function<void(const std::vector<uint8_t>&, double)>;

  ///@{
  /**
   * Start / stop listening to the UDP stream.
   * When listening, two threads are created: one for receiving and one for consuming data,
   * which then forwards the data to the callback function.
   */
  bool StartListening(const Parameters& params, const ConsumeCallback& callback);
  void StopListening();
  bool IsListening();
  ///@}

  ///@{
  /**
   * Start or stop recording the UDP stream.
   * Calling StartRecording creates a new pcap file with the specified filename.
   * Note that StartListening must be called before recording data.
   */
  void StartRecording(const std::string& filename);
  void StopRecording();
  bool IsRecording();
  ///@}

protected:
  vtkUDPPacketReceiver();
  ~vtkUDPPacketReceiver() override;

private:
  vtkUDPPacketReceiver(const vtkUDPPacketReceiver&) = delete;
  void operator=(const vtkUDPPacketReceiver&) = delete;

  std::unique_ptr<std::thread> ReceiverThread;
  std::unique_ptr<std::thread> ConsumerThread;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
