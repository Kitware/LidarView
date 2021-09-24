//=========================================================================
// Copyright 2019 Kitware, Inc.
// Author: Melanie Carriere
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


/* Method to split a pcap in two pcaps (filtering with port information)
 * ./PCAPSeparator <PcapFilePath> --Port1=? -- Port2=?
 *
 * This executable has been created to split a pcap on a virgin computer (for delivery)
 * Wireshark have similar functionalities with more options
 *
 * This executable only work for non-fragmented packets
 */

#include <vtkSmartPointer.h>

#include "Common/Network/vtkPacketFileReader.h"
#include "Common/Network/vtkPacketFileWriter.h"
#include "NetworkPacket.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

const int OUTPUT_WIDTH = 15; // width of the column (#packet, duration, ...) in the output stream

int main(int argc, char* argv[])
{
  // parse the command line options
  po::options_description visible("Allowed options");
  visible.add_options()
      ("help", "produce help message")
      ("Port1", po::value<unsigned int>()->default_value(0), "Packets send to this port in the pcap will be isolated in a second pcap")
      ("Port2", po::value<unsigned int>()->default_value(0), "Second port : 0 if you want to isolate only one port")
      ;

  po::options_description hidden("Hidden options");
  hidden.add_options()
      ("input-file", po::value<std::string>(), "input file")
      ;

  po::positional_options_description p;
  p.add("input-file", -1);

  po::options_description cmdline_options;
  cmdline_options.add(visible).add(hidden);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
            options(cmdline_options).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help") || argc < 2) {
      std::cout << "Usage: PacketFileSender <pcap_file> [options]\n";
      std::cout << visible << "\n";
      return 1;
  }

  // convert to the right type
  std::string InputFile = vm["input-file"].as<std::string>();
  unsigned int Port1 =  vm["Port1"].as<unsigned int>();
  unsigned int Port2 = vm["Port2"].as<unsigned int>();

  // Initialize Output files
  size_t lastindex = InputFile.find_last_of(".");
  std::string baseName = InputFile.substr(0, lastindex);
  std::string OutputFile1 = baseName + "_Port" + std::to_string(Port1) ;
  if(Port2 != 0)
  {
    OutputFile1 = OutputFile1 + "_" + std::to_string(Port2) ;
  }
  OutputFile1 = OutputFile1 + ".pcap";

  std::string OutputFile2 = baseName + "_OtherPorts.pcap";

  try
  {
    // Create a first Packet File Writer
    vtkPacketFileWriter writer1;
    writer1.Open(OutputFile1);

    // Create a second Packet File Writer
    vtkPacketFileWriter writer2;
    writer2.Open(OutputFile2);


    // Create a Packet File Reader
    vtkPacketFileReader* reader = new vtkPacketFileReader;

    // Variables where data will be
    const unsigned char * data = 0;
    unsigned int dataLength = 0;
    double lastPacketNetworkTime = 0;
    pcap_pkthdr* headerReference = nullptr;
    unsigned int dataHeaderLength = 0;

    if (!reader->Open(InputFile))
    {
      std::cout << "Failed to open input file" << std::endl;
      return 0;
    }

    while (reader->NextPacket(data, dataLength, lastPacketNetworkTime, &headerReference, &dataHeaderLength))
    {
      // modify the header
      auto* header = reinterpret_cast<const Ethernet_IPV4_UDP_Header*>(data-dataHeaderLength);

      if(header->upd.destination_port == Port1 || header->upd.destination_port == Port2)
      {
        writer1.WritePacket(headerReference, data - dataHeaderLength);
      }
      else
      {
        writer2.WritePacket(headerReference, data - dataHeaderLength);
      }
    }
  }
  catch (std::exception& e)
  {
    std::cout << "Caught Exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
