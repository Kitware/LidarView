/*=========================================================================

  Program: LidarView
  Module:  vtkPCAPVersion.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPCAPVersion.h"

#include <tins/tins.h>

//-----------------------------------------------------------------------------
const char* vtkPCAPVersion::GetPcapLibraryVersion()
{
  return pcap_lib_version();
}
