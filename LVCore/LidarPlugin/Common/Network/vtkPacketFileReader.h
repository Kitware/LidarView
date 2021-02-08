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
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPacketFileReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPacketFileReader -
// .SECTION Description
//

#ifndef __vtkPacketFileReader_h
#define __vtkPacketFileReader_h

#include <pcap.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_map>
#include "lidarplugin_export.h"
#include "LVTime.h"
/*
 * Useful links to get started with IPv4 and IPv6 headers:
 * - https://en.wikipedia.org/wiki/IPv4#Packet_structure
 * - https://en.wikipedia.org/wiki/IPv6_packet
 */

// Some versions of libpcap do not have PCAP_NETMASK_UNKNOWN
#if !defined(PCAP_NETMASK_UNKNOWN)
#define PCAP_NETMASK_UNKNOWN 0xffffffff
#endif

//------------------------------------------------------------------------------
// IPv6 fragment IDs are 4 bytes, IPv4 are only 2.
typedef uint32_t FragmentIdentificationT;

//------------------------------------------------------------------------------
// Packet fragment offsets are measured in blocks/steps of 8 bytes.
constexpr unsigned int FRAGMENT_OFFSET_STEP = 8;

//------------------------------------------------------------------------------
/*!
 * @brief Track fragment reassembly to confirm that all data has been
 *        reassembled before passing it on.
 */
struct FragmentTracker
{
  // The incrementally reassembled data.
  std::vector<unsigned char> Data;
  // Set when the final fragment is encountered.
  unsigned int ExpectedSize = 0;
  // Incremented as fragments are added to the data.
  unsigned int CurrentSize = 0;
};

//------------------------------------------------------------------------------
/*!
 * @brief Fragment info required for reconstructing fragmented IP packets.
 */
struct FragmentInfo
{
  FragmentIdentificationT Identification = 0;
  uint16_t Offset = 0;
  bool MoreFragments = false;

  void Reset()
  {
    this->Identification = 0;
    this->Offset = 0;
    this->MoreFragments = false;
  }
};

namespace IPHeaderFunctions
{
  //----------------------------------------------------------------------------
  /*!
   * @brief      Inspect the IP header to get fragment information.
   * @param[in]  data         A pointer to the bytes of the IP header.
   * @param[out] fragmentInfo The collected fragment info.
   * @return     True if the information could be retrieved, false otherwise.
   */
  LIDARPLUGIN_EXPORT bool getFragmentInfo(unsigned char const * data, FragmentInfo & fragmentInfo);

  //----------------------------------------------------------------------------
  /*!
   * @brief     Determine the IP header length by inspection.
   * @param[in] data A pointer to the first byte of the IP header.
   * @return    The number of bytes in the IP header, or 0 if this could not be
   *            determined.
   */
  LIDARPLUGIN_EXPORT unsigned int getIPHeaderLength(unsigned char const* data);
  
  bool buildReassembledIPHeader(unsigned char* iphdrdata, const unsigned int ipHeaderLength, const unsigned int payloadSize);
}

//------------------------------------------------------------------------------
class vtkPacketFileReader
{
public:
  vtkPacketFileReader()
  {
    this->PCAPFile = 0;
  }

  ~vtkPacketFileReader()
  {
    this->Close();
  }

  bool Open(const std::string& filename, std::string filter_arg="udp");
  bool IsOpen() { return (this->PCAPFile != 0); }
  void Close();

  const std::string& GetLastError() { return this->LastError; }
  const std::string& GetFileName() { return this->FileName; }

  void GetFilePosition(fpos_t* position)
  {
#ifdef _MSC_VER
    pcap_fgetpos(this->PCAPFile, position);
#else
    FILE* f = pcap_file(this->PCAPFile);
    fgetpos(f, position);
#endif
  }

  void SetFilePosition(fpos_t* position)
  {
#ifdef _MSC_VER
    pcap_fsetpos(this->PCAPFile, position);
#else
    FILE* f = pcap_file(this->PCAPFile);
    fsetpos(f, position);
#endif
  }

  /**
   * @brief Read next UDP payload of the capture.
   * @param[in] data A pointer to the first byte of UDP Payload.
   * @param[in] dataLength Size in bytes of the UDP Payload.
   * @param[out] timeSinceStart Network timestamp relative to the capture start time.
   * @param[out] headerReference A pointer to pointer to the Frame header (Optional).
   * @param[out] dataHeaderLength A pointer to the size of the Frame header (Optional).
   * Returns 'data' pointer to payload data of size 'dataLength'
   * Full Frame of size 'headerReference' is available,
   * header data is located at 'data - dataHeaderLength'
  */
  bool NextPacket(const unsigned char*& data, unsigned int& dataLength, double& timeSinceStart,
    pcap_pkthdr** headerReference = NULL, unsigned int* dataHeaderLength = NULL);

protected:
  pcap_t* PCAPFile;
  std::string FileName;
  std::string LastError;
  timeval StartTime;
  unsigned int FrameHeaderLength;

private:
  //! @brief A map of fragmented packet IDs to the collected array of fragments.
  std::unordered_map<FragmentIdentificationT, FragmentTracker> Fragments;

  //! @brief The ID of the last completed fragment.
  FragmentIdentificationT AssembledId = 0;

  //! @brief True if there is a reassembled packet to remove.
  bool RemoveAssembled = false;
};

#endif
