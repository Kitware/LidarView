#include "Common/Network/vtkPacketFileReader.h"

#include <iostream>
#include <algorithm>
#include <boost/endian/arithmetic.hpp>

bool IPHeaderFunctions::getFragmentInfo(unsigned char const * data, FragmentInfo & fragmentInfo)
{
  fragmentInfo.Reset();
  unsigned char const ipVersion = data[0] >> 4;
  if (ipVersion == 0x4)
  {
    fragmentInfo.Identification = 0x100 * data[4] + data[5];
    fragmentInfo.MoreFragments = (data[6] & 0x20) == 0x20;
    fragmentInfo.Offset = (data[6] & 0x1F) * 0x100 + data[7];
    return true;
  }
  if (ipVersion == 0x6)
  {
    // Scan for the fragment header.
    unsigned char nextHeader = data[6];
    data += 40;
    while (true)
    {
      switch (nextHeader)
      {
        // hop-by-hop and destination options
        case 0:
        case 60:
        // routing
        case 43:
          nextHeader = data[0];
          data += 8 + data[1];
          break;

        // fragment
        case 44:
          fragmentInfo.Offset = (data[2] * 0x100 + data[3]) >> 3;
          fragmentInfo.MoreFragments = (data[3] & 0x1) == 0x1;
          fragmentInfo.Identification = data[4] * 0x1000000 + data[5] * 0x10000 + data[6] * 0x100 + data[7];
          return true;

        // authentication header
        case 51:
          nextHeader = data[0];
          data += 2 + data[1];
          break;

        // encapsulating security payload
        case 50:
        // mobility
        case 135:
        // host identity protocol
        case 139:
        // shim6 protocol
        case 140:
        // reserved
        case 253:
        case 254:
          // None of these are currently handled.
          return false;

        default:
          return false;

      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
unsigned int IPHeaderFunctions::getIPHeaderLength(unsigned char const* data)
{
  unsigned char const ipVersion = data[0] >> 4;
  // IPv4 declares its length in the lower 4 bits of the first byte.
  if (ipVersion == 0x4)
  {
    return (data[0] & 0xf) * 4;
  }
  // IPv6 includes optional fragment headers that must be walked to determine
  // the full size.
  if (ipVersion == 0x6)
  {
    // Get the next header type.
    unsigned char nextHeader = data[6];
    // Add the length of the IPv4 header.
    unsigned char const* header = data + 40;
    // Collect the lengths of all optional headers.
    bool notPayload = true;
    while (notPayload)
    {
      switch (nextHeader)
      {
        // hop-by-hop and destination options
        case 0:
        case 60:
        // routing
        case 43:
          nextHeader = header[0];
          header += 8 + header[1];
          break;

        // fragment
        case 44:
          nextHeader = header[0];
          header += 8;
          break;

        // authentication header
        case 51:
          nextHeader = header[0];
          header += 2 + header[1];
          break;

        // encapsulating security payload
        case 50:
        // mobility
        case 135:
        // host identity protocol
        case 139:
        // shim6 protocol
        case 140:
        // reserved
        case 253:
        case 254:
          // None of these are currently handled.
          return 0;

        default:
          notPayload = false;
      }
    }
    return (header - data);
  }
  std::cerr << "IP header not recognised" << std::endl;
  return 0;
}
//------------------------------------------------------------------------------
bool IPHeaderFunctions::buildReassembledIPHeader(unsigned char* ipheader, const unsigned int ipHeaderLength, const unsigned int payloadSize)
{
  unsigned char const ipVersion = ipheader[0] >> 4;
  if (ipVersion == 0x4)
  {
    //Set Total Length
    boost::endian::big_uint16_t total_length = ipHeaderLength  + payloadSize ;
    std::copy(
      total_length.data(),
      total_length.data() + 2,
      ipheader + 2
    );
    //Reset fragment flag
    ipheader[6] =0x40;
    ipheader[7] =0x00;
    return true;
  }
  if (ipVersion == 0x6)
  {
    // WIP Removing Fragment Extension Header is a tedious process.
    // WIP This feature is not needed at the time, if ever needed
    // WIP will likely be superseded by a more robust solution like Pcap++.
    std::cerr << "IPv6 Reassembly not supported" << std::endl;
    return false;
  }
  
  std::cerr << "IP header not recognised" << std::endl;
  return false;
}

//------------------------------------------------------------------------------
bool vtkPacketFileReader::Open(const std::string& filename, std::string filter_arg, bool reassemble)
{
  //Open savefile in tcpdump/libcap format
  char errbuff[PCAP_ERRBUF_SIZE];
  pcap_t* pcapFile = pcap_open_offline(filename.c_str(), errbuff);
  if (!pcapFile)
  {
    this->LastError = errbuff;
    return false;
  }
  
  //Compute filter for the kernel-level filtering engine
  bpf_program filter;
  if (pcap_compile(pcapFile, &filter, filter_arg.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1)
  {
    this->LastError = pcap_geterr(pcapFile);
    return false;
  }
  //Associate filter to pcap
  if (pcap_setfilter(pcapFile, &filter) == -1)
  {
    this->LastError = pcap_geterr(pcapFile);
    return false;
  }

  //Determine Datalink header size
  const unsigned int loopback_header_size = 4;
  const unsigned int ethernet_header_size = 14;
  auto linktype = pcap_datalink(pcapFile);
  switch (linktype)
  {
    case DLT_EN10MB:
      this->FrameHeaderLength = ethernet_header_size;
      break;
    case DLT_NULL:
      this->FrameHeaderLength = loopback_header_size;
      break;
    default:
      this->LastError = "Unknown link type in pcap file. Cannot tell where the payload is.";
      return false;
  }

  this->FileName = filename;
  this->PCAPFile = pcapFile;
  this->StartTime.tv_sec = this->StartTime.tv_usec = 0;
  this->Reassemble = reassemble;
  return true;
}

void vtkPacketFileReader::Close()
{
  if (this->PCAPFile)
  {
    pcap_close(this->PCAPFile);
    this->PCAPFile = 0;
    this->FileName.clear();
  }
}

void vtkPacketFileReader::GetFilePosition(fpos_t* position)
{
#ifdef _MSC_VER
  pcap_fgetpos(this->PCAPFile, position);
#else
  FILE* f = pcap_file(this->PCAPFile);
  fgetpos(f, position);
#endif
}

void vtkPacketFileReader::SetFilePosition(fpos_t* position)
{
#ifdef _MSC_VER
  pcap_fsetpos(this->PCAPFile, position);
#else
  FILE* f = pcap_file(this->PCAPFile);
  fsetpos(f, position);
#endif
}
  
bool vtkPacketFileReader::NextPacket(const unsigned char*& data, unsigned int& dataLength, double& timeSinceStart,
  pcap_pkthdr** headerReference, unsigned int* dataHeaderLength )
{
  if (!this->PCAPFile)
  {
    return false;
  }

  //Clear Previous Reassembly Data if needed
  if (this->RemoveAssembled)
  {
    auto it = this->Fragments.find(this->AssembledId);
    this->Fragments.erase(it);
    this->AssembledId = 0;
    this->RemoveAssembled = false;
  }

  dataLength = 0;
  pcap_pkthdr* header;
  FragmentInfo fragmentInfo;
  fragmentInfo.MoreFragments = true;
  
  while (fragmentInfo.MoreFragments)
  {
    unsigned char const * tmpData = nullptr;
    int returnValue = pcap_next_ex(this->PCAPFile, &header, &tmpData);
    if (returnValue < 0)
    {
      this->Close();
      return false;
    }

    // Collect header values before they are removed.
    IPHeaderFunctions::getFragmentInfo(tmpData + this->FrameHeaderLength, fragmentInfo);
    timeSinceStart = GetElapsedTime(header->ts, this->StartTime);
    
    //Consts
    const unsigned int ipHeaderLength = IPHeaderFunctions::getIPHeaderLength(tmpData + this->FrameHeaderLength);
    if (ipHeaderLength == 0)
    { 
      continue;
    } 
    const unsigned int ethHeaderLength = this->FrameHeaderLength;
    const unsigned int udpHeaderLength = 8;
    
    //CaptureLen cropping
    const unsigned int frameLength    = (std::min)(header->len,header->caplen); //windows.h conflict bypass
    const unsigned int ipPayloadLength = frameLength - (ethHeaderLength + ipHeaderLength);
    
    unsigned char const * ipPayloadPtr = tmpData + ethHeaderLength + ipHeaderLength;
    
    //Provide Header Info if requested
    if (headerReference != NULL && dataHeaderLength != NULL)
    {
      *headerReference = header;
      *dataHeaderLength = ethHeaderLength + ipHeaderLength + udpHeaderLength;
    }
    
    //IP Reassembly
    if ( (fragmentInfo.MoreFragments || fragmentInfo.Offset > 0) && this->Reassemble )
    {
      const unsigned int offset_frag  = fragmentInfo.Offset * FRAGMENT_OFFSET_STEP;
      const unsigned int offset       = ethHeaderLength + ipHeaderLength + offset_frag;
      const unsigned int requiredSize = offset + ipPayloadLength;

      auto & fragmentTracker = this->Fragments[fragmentInfo.Identification];
      auto & reassembledData = fragmentTracker.Data;
      if (requiredSize > reassembledData.size())
      {
        reassembledData.resize(requiredSize);
      }
      std::copy(ipPayloadPtr, ipPayloadPtr + ipPayloadLength, reassembledData.begin() + offset);

      // Update current collected payload size.
      fragmentTracker.CurrentSize += ipPayloadLength;

      // There may be gaps of size FRAGMENT_OFFSET_STEP between fragments.
      // Add this to the current size, assumes that <number of fragments - 1> *
      // Any given fragment is always larger than FRAGMENT_OFFSET_STEP,
      // so that it cannot be omitted accidentally.
      if (fragmentInfo.MoreFragments)
      {
        // WIP I doubt this increment has any use, gap mentioned above is included within offset_frag
        // WIP commenting this line and replacing >= with == has no effects
        fragmentTracker.CurrentSize += FRAGMENT_OFFSET_STEP; 
      }else{
        // Set the expected size if this is the final fragment.
        fragmentTracker.ExpectedSize = offset_frag + ipPayloadLength;
      }

      // Return Reassembled Packet if complete.
      if (fragmentTracker.ExpectedSize > 0 && fragmentTracker.CurrentSize >= fragmentTracker.ExpectedSize)
      {
        //Build Reassembled Packet Header off the last one received
        std::copy(
          tmpData,
          ipPayloadPtr,
          reassembledData.begin()
        );
        //Frame Header
        header->len    = ethHeaderLength + ipHeaderLength  + fragmentTracker.ExpectedSize ;
        header->caplen = header->len;
        //IP Header
        IPHeaderFunctions::buildReassembledIPHeader(reassembledData.data() + ethHeaderLength, ipHeaderLength, fragmentTracker.ExpectedSize);

        // Delete the associated data on the next iteration.
        this->AssembledId = fragmentInfo.Identification;
        this->RemoveAssembled = true;
        
        //Point to dataPayload
        data       = reassembledData.data() + ethHeaderLength + ipHeaderLength + udpHeaderLength;
        dataLength = fragmentTracker.ExpectedSize - udpHeaderLength;
        return true;
      }
    }
    //Self Standing IP packet or Reassembly is not desired
    else
    {
      //Point to dataPayload
      data       = ipPayloadPtr    + udpHeaderLength;
      dataLength = ipPayloadLength - udpHeaderLength;
      return true;
    }
  }
  return true;
}
