/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarReaderProxy.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMLidarReaderProxy.h"

#include <vtkClientServerStream.h>
#include <vtkObjectFactory.h>
#include <vtkSMSession.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMLidarReaderProxy);

//----------------------------------------------------------------------------
vtkSMLidarReaderProxy::vtkSMLidarReaderProxy() = default;

//----------------------------------------------------------------------------
vtkSMLidarReaderProxy::~vtkSMLidarReaderProxy() = default;

//----------------------------------------------------------------------------
void vtkSMLidarReaderProxy::SaveFrames(unsigned int start,
  unsigned int end,
  const std::string& filename)
{
  auto session = this->GetSession();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SaveFrames" << start << end
         << filename << vtkClientServerStream::End;
  session->ExecuteStream(vtkPVSession::DATA_SERVER_ROOT, stream, /*ignore errors*/ true);
}

//----------------------------------------------------------------------------
void vtkSMLidarReaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
