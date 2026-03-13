/*=========================================================================

  Program: LidarView
  Module:  vtkPCAPVersion.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPCAPVersion_h
#define vtkPCAPVersion_h

#include "lvIONetworkModule.h"

/**
 * Utility functions to get the underlying pcap library version.
 */
namespace vtkPCAPVersion
{
LVIONETWORK_EXPORT const char* GetPcapLibraryVersion();
};

#endif
