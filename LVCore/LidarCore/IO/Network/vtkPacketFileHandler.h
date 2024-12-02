/*=========================================================================

  Program: LidarView
  Module:  vtkPacketFileHandler.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPacketFileHandler_h
#define vtkPacketFileHandler_h

// Compliance with vtk's fpos_t policy, needs to be included before any libc header
#include "vtkPacketFilePositionType.h"

#include "lvIONetworkModule.h"
#include <vtkObject.h>

#include <cstdint>
#include <memory>

/**
 * vtkPacketFileHandler is used to open and close pcap files,
 * read their contents, and rewrite portions of the file to a new file.
 *
 * Note that its currently reading only UPD packets.
 */
class LVIONETWORK_EXPORT vtkPacketFileHandler : public vtkObject
{
public:
  static vtkPacketFileHandler* New();
  vtkTypeMacro(vtkPacketFileHandler, vtkObject);

  ///@{
  /**
   * Open / close pcap file.
   */
  bool Open(const std::string& filename, std::string filterArgs = "udp");
  bool IsOpen() const;
  void Close();
  ///@}

  ///@{
  /**
   * Get / Set the current position within the pcap file.
   * This can be used for random access operations.
   */
  void GetFilePosition(vtkPcapIdxType* position) const;
  void SetFilePosition(vtkPcapIdxType* position);
  ///@}

  /**
   * Read the next packet from the pcap file.
   *
   * The pcap file must be opened with `Open()` first.
   * After reading a packet, use `GetPayload()` and `GetTimestamp()` to retrieve
   * its data and timestamp. Automatic pcap reassembly is performed in this method.
   */
  bool ReadNextPacket();

  /**
   * Get the payload of the last packet read.
   */
  const std::vector<uint8_t>& GetPayload() const;

  /**
   * Get the timestamp of the last packet read.
   */
  double GetTimestamp() const;

  /**
   * Write packets within a specific range to a new pcap file.
   * It takes the start pcap position and stop recording either when whole pcap is written
   * or when the current packet timestamp is greater than endNetworkTime.
   *
   * The pcap file must be opened with `Open()` first. This method uses
   * `SetFilePosition()` internally to iterate through the pcap file, so it
   * should not be used concurrently with `ReadNextPacket()`.
   */
  void WritePackets(std::string filename, vtkPcapIdxType* startPosition, double endNetworkTime);

protected:
  vtkPacketFileHandler();
  ~vtkPacketFileHandler() override;

private:
  vtkPacketFileHandler(const vtkPacketFileHandler&) = delete;
  void operator=(const vtkPacketFileHandler&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
