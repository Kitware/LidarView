#include "NetworkPacket.h"
#include <cstring>
#include <iostream>

#include <boost/endian/arithmetic.hpp>
#ifdef _MSC_VER
#include <windows.h>

namespace
{
//------------------------------------------------------------------------------
int gettimeofday(struct timeval* tp, void*)
{
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  long long t = (static_cast<long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
  t -= 116444736000000000LL;
  t /= 10; // microseconds
  tp->tv_sec = static_cast<long>(t / 1000000UL);
  tp->tv_usec = static_cast<long>(t % 1000000UL);
  return 0;
}
}

#endif

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


//------------------------------------------------------------------------------
NetworkPacket* NetworkPacket::BuildEthernetIP4UDP(const unsigned char* payload,
                                                 std::size_t payloadSize,
                                                 std::array<unsigned char, 4> sourceIPv4BigEndian,
                                                 uint16_t sourcePort,
                                                 uint16_t destinationPort,
{
  NetworkPacket* packet = new NetworkPacket();
  gettimeofday(&packet->ReceptionTime, nullptr);

  packet->PacketData.resize(sizeof(Ethernet_IPV4_UDP_Header) + payloadSize);
  packet->PayloadStart = sizeof(Ethernet_IPV4_UDP_Header);

  // copy payload to packet
  std::copy(payload,
            payload + payloadSize,
            packet->PacketData.begin() + sizeof(Ethernet_IPV4_UDP_Header));

  // modify the header
  auto* header = reinterpret_cast<Ethernet_IPV4_UDP_Header*>(packet->PacketData.data());
  *header = Ethernet_IPV4_UDP_Header();

  // UDP
  header->upd.source_port = sourcePort;
  header->upd.destination_port = destinationPort;
  header->upd.length = sizeof(upd_header) + payloadSize;

  // IPV4
  header->ipv4.source_ip_address = *reinterpret_cast<boost::endian::big_uint32_t *>(sourceIPv4BigEndian.data());
  header->ipv4.total_length = sizeof(ipv4_header) + sizeof(upd_header) + payloadSize;

  // We could compute and set the UDP checksum, then the IP checksum but this
  // was not done in the past.

  return packet;
}

//------------------------------------------------------------------------------
const unsigned char* NetworkPacket::GetPacketData() const
{
  return this->PacketData.data();
}

//------------------------------------------------------------------------------
unsigned int NetworkPacket::GetPacketSize() const
{
  return this->PacketData.size();
}

//------------------------------------------------------------------------------
const unsigned char* NetworkPacket::GetPayloadData() const
{
  return this->PacketData.data() + this->PayloadStart;
}

//------------------------------------------------------------------------------
unsigned int NetworkPacket::GetPayloadSize() const
{
  // last - first + 1
  return (this->GetPacketSize() - 1) - this->PayloadStart + 1;
}
