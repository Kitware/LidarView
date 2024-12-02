/*=========================================================================

  Program: LidarView
  Module:  vtkPacketFilePositionHandler.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPacketFilePositionHandler.h"

#include <cstdio>

#if defined(LIBPCAP_INDEXING_SUPPORTED)

#if defined(_WIN32)
#define FTELL _ftelli64
#define FSEEK _fseeki64
#else
#define FTELL std::ftell
#define FSEEK std::fseek
#endif

//-----------------------------------------------------------------------------
void vtkPacketFilePositionHandler::SetFilePosition(pcap_t* file, int64_t* position)
{
  int64_t pcapHeaderSize = sizeof(struct pcap_file_header);
  if (*position < pcapHeaderSize)
  {
    *position = pcapHeaderSize;
  }
  FILE* fd = pcap_file(file);
  FSEEK(fd, *position, SEEK_SET);
}

//-----------------------------------------------------------------------------
void vtkPacketFilePositionHandler::GetFilePosition(pcap_t* file, int64_t* position)
{
  FILE* fd = pcap_file(file);
  *position = FTELL(fd);
}

#else

//-----------------------------------------------------------------------------
void vtkPacketFilePositionHandler::SetFilePosition(pcap_t* file, fpos_t* position)
{
#if defined(_WIN32)
  // Custom method from pat marion winpcap fork
  pcap_fsetpos(file, position);
#else
  FILE* fd = pcap_file(file);
  fsetpos(fd, position);
#endif
}

//-----------------------------------------------------------------------------
void vtkPacketFilePositionHandler::GetFilePosition(pcap_t* file, fpos_t* position)
{
#if defined(_WIN32)
  // Custom method from pat marion winpcap fork
  pcap_fgetpos(file, position);
#else
  FILE* fd = pcap_file(file);
  fgetpos(fd, position);
#endif
}

#endif