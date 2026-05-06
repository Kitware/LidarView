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

#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include <tins/tins.h>

#include <cstdio>
#include <memory>

#if defined(_WIN32)
#define FTELL _ftelli64
#define FSEEK _fseeki64
#else
#define FTELL std::ftell
#define FSEEK std::fseek
#endif

namespace
{
constexpr unsigned int PROTOCOL_UDP = 17;
constexpr double US_TO_S_FACTOR = 1e-6;

#if defined(_WIN32)
//-----------------------------------------------------------------------------
bool Win32DuplicateHandle(HANDLE original, HANDLE& copy)
{
  BOOL duplicated = DuplicateHandle(
    GetCurrentProcess(), original, GetCurrentProcess(), &copy, 0, FALSE, DUPLICATE_SAME_ACCESS);
  if (!duplicated || copy == INVALID_HANDLE_VALUE)
  {
    vtkWarningWithObjectMacro(nullptr, "Could not duplicate file handle");
    return false;
  }
  return true;
}
#endif

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
#if defined(_WIN32)
  //-----------------------------------------------------------------------------
  lvFileSniffer(HANDLE handle, const lvSnifferConfiguration& configuration)
  {
    char error[PCAP_ERRBUF_SIZE];
    pcap_t* phandle = pcap_hopen_offline(reinterpret_cast<intptr_t>(handle), error);
    if (!phandle)
    {
      vtkWarningWithObjectMacro(nullptr, "Could not open pcap: " << error);
      CloseHandle(handle); // pcap failed, we must close since it didn't take ownership
      handle = INVALID_HANDLE_VALUE;
      return;
    }
    this->SetPcapHandle(phandle, configuration);
  }
#else
  //-----------------------------------------------------------------------------
  lvFileSniffer(FILE* fp, const lvSnifferConfiguration& configuration)
  {
    char error[PCAP_ERRBUF_SIZE];
    pcap_t* phandle = pcap_fopen_offline(fp, error);
    if (!phandle)
    {
      vtkWarningWithObjectMacro(nullptr, "Could not open pcap: " << error);
      return;
    }
    this->SetPcapHandle(phandle, configuration);
  }
#endif

  //-----------------------------------------------------------------------------
  void SetPcapHandle(pcap_t* phandle, const lvSnifferConfiguration& configuration)
  {
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
#if defined(_WIN32)
  HANDLE Win32Handle = INVALID_HANDLE_VALUE;
#endif

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

#if defined(_WIN32)
  {
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    HANDLE original = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(internals.FileHandle)));
    // Our copy — closed independently on cleanup
    if (!::Win32DuplicateHandle(original, internals.Win32Handle))
    {
      return false;
    }
    // libtins/libpcap's copy — owned and closed by pcap_close()
    HANDLE pcapHandle = INVALID_HANDLE_VALUE;
    if (!::Win32DuplicateHandle(original, pcapHandle))
    {
      CloseHandle(internals.Win32Handle);
      internals.Win32Handle = INVALID_HANDLE_VALUE;
      return false;
    }
    internals.PCAPReader = std::make_unique<lvFileSniffer>(pcapHandle, config);
  }
#else
  internals.PCAPReader = std::make_unique<lvFileSniffer>(internals.FileHandle, config);
#endif
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
    internals.PCAPFileHandle = nullptr;
    internals.PCAPReader.reset(); // pcap_close() in tins closes pcap's duplicated HANDLE

#if defined(_WIN32)
    if (internals.Win32Handle != INVALID_HANDLE_VALUE)
    {
      CloseHandle(internals.Win32Handle);
      internals.Win32Handle = INVALID_HANDLE_VALUE;
    }
#endif

    if (internals.FileHandle)
    {
      // On Windows close the file handle, on linux tins already closed it.
#if defined(_WIN32)
      std::fclose(internals.FileHandle);
#endif
      internals.FileHandle = nullptr;
    }
  }
}

//------------------------------------------------------------------------------
int64_t vtkPacketFileHandler::GetFilePosition()
{
  if (!this->IsOpen())
  {
    return 0;
  }
  auto& internals = *this->Internals;

  FILE* fd = pcap_file(internals.PCAPFileHandle);
  int64_t position = FTELL(fd);
  if (position == -1)
  {
    vtkErrorMacro("Ftell error!");
    this->Close();
    return 0;
  }
  return position;
}

//------------------------------------------------------------------------------
void vtkPacketFileHandler::SetFilePosition(int64_t position)
{
  if (!this->IsOpen())
  {
    return;
  }
  auto& internals = *this->Internals;

  int64_t pcapHeaderSize = sizeof(struct pcap_file_header);
  if (position < pcapHeaderSize)
  {
    position = pcapHeaderSize;
  }
  FILE* fd = pcap_file(internals.PCAPFileHandle);
  FSEEK(fd, position, SEEK_SET);
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
    // NOTE: In rare cases, the first packet in a pcap file may not be read on the initial call,
    // requiring multiple calls to `next_packet` to retrieve it.
    // For code clarity and maintainability, this edge case is not explicitly handled here.
    // If this issue recurs, refer to the fix proposed in lidarview-core!653.
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
  int64_t startPosition,
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

//------------------------------------------------------------------------------
std::set<uint16_t> vtkPacketFileHandler::GetAvailableUDPPort(std::string filename)
{
  std::set<uint16_t> ports;
  try
  {
    std::unique_ptr<Tins::FileSniffer> sniffer = std::make_unique<Tins::FileSniffer>(filename);
    while (Tins::Packet pkt = sniffer->next_packet())
    {
      Tins::PDU* pdu = pkt.pdu();
      if (pdu)
      {
        Tins::IP* ip = pdu->find_pdu<Tins::IP>();
        Tins::IPv6* ipv6 = pdu->find_pdu<Tins::IPv6>();
        if ((ip && ip->protocol() == ::PROTOCOL_UDP) ||
          (ipv6 && ipv6->next_header() == ::PROTOCOL_UDP))
        {
          Tins::UDP* udp = pdu->find_pdu<Tins::UDP>();
          if (udp != nullptr)
          {
            ports.emplace(udp->sport());
          }
        }
      }
    }
  }
  catch (Tins::pcap_error&)
  {
    vtkLog(ERROR, "Could not open file: " << filename);
  }
  return ports;
}
