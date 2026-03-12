/*=========================================================================

  Program: LidarView
  Module:  vtkDummyExternalAPI.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDummyExternalAPI.h"

#include "zmqInfo.h"

#include <sstream>

#ifdef ZeroMQ_FOUND
#include "zmq.h"
#endif

std::string vtkDummyExternalAPI::GetVersion()
{
#ifdef ZeroMQ_FOUND
  int major, minor, patch = 0;
  zmq_version(&major, &minor, &patch);

  std::stringstream ss;
  ss << "v" << major << "." << minor << "." << patch;
  return ss.str();
#else
  return "Library not found!";
#endif
}
