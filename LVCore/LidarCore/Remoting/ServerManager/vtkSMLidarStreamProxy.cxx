/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarStreamProxy.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMLidarStreamProxy.h"

#include <vtkClientServerStream.h>
#include <vtkObjectFactory.h>
#include <vtkPVSession.h>
#include <vtkSMSession.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMLidarStreamProxy);

//----------------------------------------------------------------------------
vtkSMLidarStreamProxy::vtkSMLidarStreamProxy() = default;

//----------------------------------------------------------------------------
vtkSMLidarStreamProxy::~vtkSMLidarStreamProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMLidarStreamProxy::DoesNeedsUpdate()
{
  auto session = this->GetSession();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "GetNeedsUpdate"
         << vtkClientServerStream::End;
  session->ExecuteStream(vtkPVSession::DATA_SERVER_ROOT, stream, /*ignore errors*/ true);

  vtkClientServerStream result = session->GetLastResult(vtkPVSession::DATA_SERVER_ROOT);
  bool needs_update = false;
  if (result.GetNumberOfMessages() == 1 && result.GetNumberOfArguments(0) == 1)
  {
    result.GetArgument(0, 0, &needs_update);
  }
  return needs_update;
}

//----------------------------------------------------------------------------
void vtkSMLidarStreamProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
