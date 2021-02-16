#include "NetworkPacket.h"
#include <cstring>
#include <iostream>

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

//------------------------------------------------------------------------------
NetworkPacket* NetworkPacket::BuildEthernetIP4UDP(const unsigned char* payload,
                                                 std::size_t payloadSize,
                                                 std::array<unsigned char, 4> sourceIPv4BigEndian,
                                                 uint16_t sourcePort,
                                                 uint16_t destinationPort,
                                                 uint64_t macAddress)
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

  header->ethernet.source_mac_address = macAddress;

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
