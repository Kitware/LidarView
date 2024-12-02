/*=========================================================================

  Program: LidarView
  Module:  vtkPacketFilePositionHandler.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPacketFilePositionHandler_h
#define vtkPacketFilePositionHandler_h

// Compliance with vtk's fpos_t policy, needs to be included before any libc header
#include "vtkPacketFilePositionType.h"

#include <tins/tins.h>

/**
 * Utility functions for setting the current reading index of the pcap pointer.
 * The index uses either `fpos_t` or `int64_t`, depending on the definition of `vtkPcapIdxType`.
 *
 * These functions are necessary due to differences in the APIs of WinPcap and Npcap.
 */
namespace vtkPacketFilePositionHandler
{
void SetFilePosition(pcap_t* file, vtkPcapIdxType* position);
void GetFilePosition(pcap_t* file, vtkPcapIdxType* position);
};

#endif
