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

#include <boost/endian/arithmetic.hpp>

#pragma pack(push, 1)
struct ethernet_header
{
  boost::endian::big_uint48_t destination_mac_address {0xffffffffffff};
  boost::endian::big_uint48_t source_mac_address {0xffffffffffff};
  boost::endian::big_uint16_t packet_type {0x0800}; // IP
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ipv4_header
{
  boost::endian::big_uint8_t version_and_IHL {0x45};
  boost::endian::big_uint8_t service {0x00};
  boost::endian::big_uint16_t total_length {0x04d2};
  boost::endian::big_uint16_t identification {0x0000};
  boost::endian::big_uint16_t flag_and_fragment_offset {0x4000}; // do not fragment
  boost::endian::big_uint8_t time_to_live {0xff};
  boost::endian::big_uint8_t protocol {0x11}; // UDP
  boost::endian::big_uint16_t checksum {0xffff};
  boost::endian::big_uint32_t source_ip_address {0xc0a801c8}; // 192.168.1.200
  boost::endian::big_uint32_t destination_ip_address {0xffffffff}; // broadcast
};
#pragma pack(pop)

#pragma pack(push, 1)
struct upd_header
{
  boost::endian::big_uint16_t source_port {2368};
  boost::endian::big_uint16_t destination_port {2368};
  boost::endian::big_uint16_t length {0x04be};
  boost::endian::big_uint16_t checksum {0x0000};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Ethernet_IPV4_UDP_Header
{
  ethernet_header ethernet;
  ipv4_header ipv4;
  upd_header upd;
};
#pragma pack(pop)

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
