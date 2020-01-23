//=========================================================================
// Copyright 2019 Kitware, Inc.
// Author: Gabriel Devillers
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
//=========================================================================

#ifndef NETWORKPACKET_H
#define NETWORKPACKET_H

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <sys/time.h>
#endif
#include <vector>
#include <array>

class NetworkPacket
{
public:
  static NetworkPacket* BuildEthernetIP4UDP(const unsigned char* payload,
                                           std::size_t payloadSize,
                                           std::array<unsigned char, 4> sourceIPv4BigEndian,
                                           uint16_t sourcePort,
                                           uint16_t destinationPort,
                                           uint64_t macAddress);

  // Note that the memory zone returned by P->GetPayloadData() has a lifetime equal
  // to the instance P of NetworkPacket (no copy is done), and must not be freed
  // by the user of NetworkPacket
  const unsigned char* GetPacketData() const;
  unsigned int GetPacketSize() const;
  const unsigned char* GetPayloadData() const;
  unsigned int GetPayloadSize() const;
  struct timeval ReceptionTime;

private:
  NetworkPacket() = default; // prevent construction without using a static constructor
  unsigned int PayloadStart = 0;
  // should you want to switch PacketData to a C array,
  // ensure care that the copy constructor copies it.
  std::vector<unsigned char> PacketData;
};


#endif // NETWORKPACKET_H
