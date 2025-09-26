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

// Compliance with vtk's fpos_t policy, needs to be included before any libc header
#include "vtkPacketFilePositionHandler.h"

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

/**
 * Inherit from Tins::SnifferConfiguration to expose protected properties
 * and avoid friend IDIOM.
 */
class lvSnifferConfiguration : public Tins::SnifferConfiguration
{
public:
  lvSnifferConfiguration() = default;

  uint32_t IsFlagValid() const { return flags_ & Tins::SnifferConfiguration::PACKET_FILTER; };
  std::string GetFilter() const { return filter_; };
  Tins::BaseSniffer::PcapSniffingMethod GetSniffingMethod() const { return pcap_sniffing_method_; };
};

class lvFileSniffer : public Tins::BaseSniffer
{
public:
  //-----------------------------------------------------------------------------
  lvFileSniffer(FILE* fp, const lvSnifferConfiguration& configuration)
  {
    char error[PCAP_ERRBUF_SIZE];
    // Maybe useful later
    // pcap_t* phandle = pcap_hopen_offline(_get_osfhandle(_fileno(fp)), err_buf);
    pcap_t* phandle = pcap_fopen_offline(fp, error);
    if (!phandle)
    {
      vtkWarningWithObjectMacro(nullptr, "Could not open pcap: " << error);
      return;
    }

    this->set_pcap_handle(phandle);
    if (configuration.IsFlagValid() != 0)
    {
      if (!this->set_filter(configuration.GetFilter()))
      {
        vtkWarningWithObjectMacro(
          nullptr, "Could not open pcap: " << pcap_geterr(this->get_pcap_handle()));
      }
    }
    this->set_pcap_sniffing_method(configuration.GetSniffingMethod());
  }
};

}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPacketFileHandler);

//-----------------------------------------------------------------------------
class vtkPacketFileHandler::vtkInternals
{
public:
  std::unique_ptr<lvFileSniffer> PCAPReader;
  FILE* FileHandle = nullptr;
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

  lvSnifferConfiguration config;
  config.set_filter(filterArgs);

  internals.FileHandle = std::fopen(filename.c_str(), "rb");
  internals.PCAPReader = std::make_unique<lvFileSniffer>(internals.FileHandle, config);
  internals.PCAPFileHandle = internals.PCAPReader->get_pcap_handle();
  return this->IsOpen();
}

//------------------------------------------------------------------------------
bool vtkPacketFileHandler::IsOpen() const
{
  auto& internals = *this->Internals;
  if (internals.PCAPReader && internals.PCAPFileHandle && internals.FileHandle)
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
    if (internals.PCAPFileHandle)
    {
      internals.PCAPFileHandle = nullptr;
    }
    if (internals.FileHandle)
    {
      // For some reason the pcap isn't fully closed on Windows with the pcap_close call (in tins)
      // closing the file handler seems to do the trick but it throw a warning on debug mode...
#if defined(_WIN32)
      std::fclose(internals.FileHandle);
#endif
      internals.FileHandle = nullptr;
    }
  }
}

//------------------------------------------------------------------------------
void vtkPacketFileHandler::GetFilePosition(vtkPcapIdxType* position) const
{
  if (!this->IsOpen())
  {
    return;
  }
  auto& internals = *this->Internals;
  vtkPacketFilePositionHandler::GetFilePosition(internals.PCAPFileHandle, position);
}

//------------------------------------------------------------------------------
void vtkPacketFileHandler::SetFilePosition(vtkPcapIdxType* position)
{
  if (!this->IsOpen())
  {
    return;
  }
  auto& internals = *this->Internals;
  vtkPacketFilePositionHandler::SetFilePosition(internals.PCAPFileHandle, position);
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
  vtkPcapIdxType* startPosition,
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
