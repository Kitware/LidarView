/*=========================================================================

  Program: LidarView
  Module:  vtkStreamPacketHandler.txx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkStreamPacketHandler_txx
#define vtkStreamPacketHandler_txx

#include "vtkStreamPacketHandler.h"

//-----------------------------------------------------------------------------
template <typename T>
std::string vtkStreamPacketHandler::BuildPCAPFilter(std::string protocol,
  std::string host,
  std::vector<T> ports)
{
  std::string filter = protocol;
  if (!host.empty())
  {
    filter += " and src host " + host;
  }
  std::string portsStr;
  for (size_t idx = 0; idx < ports.size(); idx++)
  {
    unsigned int port = ports.at(idx);
    portsStr.append("port " + std::to_string(port));
    if (idx < ports.size() - 1)
    {
      portsStr.append(" or ");
    }
  }
  if (!portsStr.empty())
  {
    filter += " and (" + portsStr + ")";
  }
  return filter;
}

#endif
