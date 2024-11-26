/*=========================================================================

  Program: LidarView
  Module:  vtkPacketFileHandler.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPacketFileHandler.h"

#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include <tins/tins.h>

#include <cstdio>
#include <memory>

namespace
{
constexpr unsigned int PROTOCOL_UDP = 17;
constexpr double US_TO_S_FACTOR = 1e-6;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPacketFileHandler);

//-----------------------------------------------------------------------------
class vtkPacketFileHandler::vtkInternals
{
public:
  std::unique_ptr<Tins::FileSniffer> PCAPReader;
  pcap_t* PCAPFileHandle = nullptr;
  uint64_t FileSize = 0;
  Tins::Packet PktCache;
  std::vector<uint8_t> CurrentPayload;
};

//------------------------------------------------------------------------------
vtkPacketFileHandler::vtkPacketFileHandler()
  : Internals(new vtkPacketFileHandler::vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkPacketFileHandler::~vtkPacketFileHandler() = default;

//------------------------------------------------------------------------------
bool vtkPacketFileHandler::Open(const std::string& filename, std::string filterArgs)
{
  auto& internals = *this->Internals;
  std::ifstream fileSizeStream(filename, std::ios::binary);
  if (!fileSizeStream)
  {
    return false;
  }
  fileSizeStream.seekg(0, std::ios::end);
  internals.FileSize = fileSizeStream.tellg();
  fileSizeStream.close();

  Tins::SnifferConfiguration config;
  config.set_filter(filterArgs);

  internals.PCAPReader = std::make_unique<Tins::FileSniffer>(filename, config);
  internals.PCAPFileHandle = internals.PCAPReader->get_pcap_handle();
  return this->IsOpen();
}

//------------------------------------------------------------------------------
bool vtkPacketFileHandler::IsOpen() const
{
  auto& internals = *this->Internals;
  if (internals.PCAPReader && internals.PCAPFileHandle)
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkPacketFileHandler::Close()
{
  if (this->IsOpen())
  {
    auto& internals = *this->Internals;
    internals.PCAPReader.reset();
    internals.PCAPFileHandle = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkPacketFileHandler::GetFilePosition(fpos_t* position) const
{
  if (!this->IsOpen())
  {
    return;
  }
  auto& internals = *this->Internals;

#ifdef _MSC_VER
  // Custom method from pat marion winpcap fork
  pcap_fgetpos(internals.PCAPFileHandle, position);
#else
  FILE* f = pcap_file(internals.PCAPFileHandle);
  fgetpos(f, position);
#endif
}

//------------------------------------------------------------------------------
void vtkPacketFileHandler::SetFilePosition(fpos_t* position)
{
  if (!this->IsOpen())
  {
    return;
  }
  auto& internals = *this->Internals;

#ifdef _MSC_VER
  // Custom method from pat marion winpcap fork
  pcap_fsetpos(internals.PCAPFileHandle, position);
#else
  FILE* f = pcap_file(internals.PCAPFileHandle);
  fsetpos(f, position);
#endif
}

//------------------------------------------------------------------------------
bool vtkPacketFileHandler::ReadNextPacket()
{
  if (!this->IsOpen())
  {
    return false;
  }

  auto& internals = *this->Internals;

  bool isReassembled = false;
  while (!isReassembled)
  {
    internals.PktCache = internals.PCAPReader->next_packet();
    if (!internals.PktCache)
    {
      return false;
    }
    Tins::PDU* pdu = internals.PktCache.pdu();
    if (!pdu)
    {
      continue;
    }
    Tins::IP* ip = pdu->find_pdu<Tins::IP>();
    Tins::IPv6* ipv6 = pdu->find_pdu<Tins::IPv6>();
    if ((ip && ip->protocol() == ::PROTOCOL_UDP) || (ipv6 && ipv6->next_header() == ::PROTOCOL_UDP))
    {
      Tins::IPv4Reassembler reassembler;
      isReassembled = reassembler.process(*pdu) != Tins::IPv4Reassembler::FRAGMENTED;
      if (isReassembled)
      {
        // find_pdu<Tins::UDP> will only return NULL when the ipv4
        // reassembly succeeds on an ipv6 packet, leading to a
        // malformed packet
        Tins::UDP* udp = pdu->find_pdu<Tins::UDP>();
        if (udp != nullptr)
        {
          auto raw = pdu->find_pdu<Tins::RawPDU>();
          internals.CurrentPayload = raw->payload();
        }
        else
        {
          break;
        }
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkPacketFileHandler::WritePackets(std::string filename,
  fpos_t* startPosition,
  double endNetworkTime)
{
  auto& internals = *this->Internals;
  try
  {
    Tins::PacketWriter writer(filename, Tins::DataLinkType<Tins::EthernetII>());

    this->SetFilePosition(startPosition);
    while (this->IsOpen())
    {
      internals.PktCache = internals.PCAPReader->next_packet();
      if (!internals.PktCache || this->GetTimestamp() > endNetworkTime)
      {
        break;
      }
      writer.write(internals.PktCache);
    }
  }
  catch (const std::exception& ex)
  {
    vtkErrorMacro("Failed to write packet: " << ex.what());
  }
}

//------------------------------------------------------------------------------
const std::vector<uint8_t>& vtkPacketFileHandler::GetPayload() const
{
  auto& internals = *this->Internals;
  return internals.CurrentPayload;
}

//------------------------------------------------------------------------------
double vtkPacketFileHandler::GetTimestamp() const
{
  auto& internals = *this->Internals;
  return internals.PktCache.timestamp().seconds() +
    internals.PktCache.timestamp().microseconds() * ::US_TO_S_FACTOR;
}
