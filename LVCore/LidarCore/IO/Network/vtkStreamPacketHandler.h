/*=========================================================================

  Program: LidarView
  Module:  vtkStreamPacketHandler.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkStreamPacketHandler_h
#define vtkStreamPacketHandler_h

#include "lvIONetworkModule.h"
#include <vtkObject.h>

#include <functional>
#include <string>

class vtkPacketRecorder;

/**
 * Is an interface class to implement different packet handler that
 * can be used by a vtkStream.
 *
 * @sa vtkUPDPacketReceiver
 */
class LVIONETWORK_EXPORT vtkStreamPacketHandler : public vtkObject
{
public:
  vtkTypeMacro(vtkStreamPacketHandler, vtkObject);

  using ConsumeCallback = std::function<void(const std::vector<uint8_t>&, double)>;

  ///@{
  /**
   * Start / stop listening.
   */
  virtual bool StartListening(const std::vector<unsigned int>& ports,
    const ConsumeCallback& callback) = 0;
  virtual void StopListening() = 0;
  virtual bool IsListening() = 0;
  ///@}

  /**
   * If set the packet receiver will forward received packets to the recorder.
   */
  virtual void SetRecorder(vtkPacketRecorder* writer) = 0;

  /**
   * Constructs a pcap filter string that filters packets based on the specified
   * protocol, source host, and a set of ports.

   * Example usage:
   * Given protocol = "udp", host = "127.0.0.1", and ports = {8000, 8001},
   * the function returns:
   *   "udp and src host 127.0.0.1 and (port 8000 or port 8001)"
   */
  template <typename T>
  static std::string BuildPCAPFilter(std::string protocol, std::string host, std::vector<T> ports);

protected:
  vtkStreamPacketHandler() = default;
  ~vtkStreamPacketHandler() override = default;

private:
  vtkStreamPacketHandler(const vtkStreamPacketHandler&) = delete;
  void operator=(const vtkStreamPacketHandler&) = delete;
};

#include "vtkStreamPacketHandler.txx"

#endif
