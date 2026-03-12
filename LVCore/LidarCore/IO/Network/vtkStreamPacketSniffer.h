/*=========================================================================

  Program: LidarView
  Module:  vtkStreamPacketSniffer.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkStreamPacketSniffer_h
#define vtkStreamPacketSniffer_h

#include "lvIONetworkModule.h"
#include "vtkStreamPacketHandler.h"

#include <vtkSetGet.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

class vtkPacketRecorder;

/**
 * vtkStreamPacketSniffer
 */
class LVIONETWORK_EXPORT vtkStreamPacketSniffer : public vtkStreamPacketHandler
{
public:
  static vtkStreamPacketSniffer* New();
  vtkTypeMacro(vtkStreamPacketSniffer, vtkStreamPacketHandler);

  ///@{
  /**
   * The network interface name to sniff packet on.
   * If empty all interfaces are listen.
   */
  vtkGetMacro(NetworkInterface, std::string);
  vtkSetMacro(NetworkInterface, std::string);
  ///@}

  ///@{
  /**
   * The host address to search for, if empty all address are listen.
   */
  vtkGetMacro(HostAddress, std::string);
  vtkSetMacro(HostAddress, std::string);
  ///@}

  ///@{
  /**
   * Start / stop listening to the UDP stream.
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
  vtkStreamPacketSniffer();
  ~vtkStreamPacketSniffer() override;

private:
  vtkStreamPacketSniffer(const vtkStreamPacketSniffer&) = delete;
  void operator=(const vtkStreamPacketSniffer&) = delete;

  std::string NetworkInterface;
  std::string HostAddress;

  std::unique_ptr<std::thread> ConsumerThread;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
