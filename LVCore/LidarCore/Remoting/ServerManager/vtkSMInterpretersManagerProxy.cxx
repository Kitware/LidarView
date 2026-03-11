/*=========================================================================

  Program: LidarView
  Module:  vtkSMInterpretersManagerProxy.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMInterpretersManagerProxy.h"

#include <vtkObjectFactory.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyListDomain.h>
#include <vtkSMSessionProxyManager.h>

namespace
{
constexpr const char* PROXY_GROUP_NAME = "lidar_interpreters";
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMInterpretersManagerProxy);

//----------------------------------------------------------------------------
vtkSMInterpretersManagerProxy::vtkSMInterpretersManagerProxy() = default;

//----------------------------------------------------------------------------
vtkSMInterpretersManagerProxy::~vtkSMInterpretersManagerProxy() = default;

//----------------------------------------------------------------------------
unsigned int vtkSMInterpretersManagerProxy::GetNumberOfInterpreters()
{
  const auto pld = this->GetProperty("Interpreter")->FindDomain<vtkSMProxyListDomain>();
  return pld->GetProxyTypes().size();
}

//----------------------------------------------------------------------------
const char* vtkSMInterpretersManagerProxy::GetInterpreterName(unsigned int idx)
{
  const auto pld = this->GetProperty("Interpreter")->FindDomain<vtkSMProxyListDomain>();
  if (idx >= pld->GetProxyTypes().size())
  {
    return "";
  }
  return pld->GetProxyTypes().at(idx).ProxyName.c_str();
}

//----------------------------------------------------------------------------
std::vector<const char*> vtkSMInterpretersManagerProxy::GetAvailableInterpreters()
{
  std::vector<const char*> interpreters;
  const auto pld = this->GetProperty("Interpreter")->FindDomain<vtkSMProxyListDomain>();
  for (const auto& proxyType : pld->GetProxyTypes())
  {
    interpreters.emplace_back(proxyType.ProxyName.c_str());
  }
  return interpreters;
}

//----------------------------------------------------------------------------
const char* vtkSMInterpretersManagerProxy::GetLidarProxyName(vtkSMProxy* interpreter,
  const char* propertyReaderName,
  const char* propertyStreamName)
{
  auto mode = vtkSMPropertyHelper(this->GetProperty("Mode")).GetAsInt();

  switch (mode)
  {
    case Mode::READER:
      return vtkSMPropertyHelper(interpreter, propertyReaderName).GetAsString();
    case Mode::STREAM:
      return vtkSMPropertyHelper(interpreter, propertyStreamName).GetAsString();
    default:
      vtkErrorMacro("Interpretion mode not implemented.");
      return nullptr;
  }
}

//----------------------------------------------------------------------------
const char* vtkSMInterpretersManagerProxy::GetLidarProxyName(const char* interpreter,
  unsigned int type)
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  auto interpreterProxy = pxm->GetPrototypeProxy(::PROXY_GROUP_NAME, interpreter);
  if (!interpreterProxy)
  {
    vtkErrorMacro("Could not get prototype proxy: sm_group=" << ::PROXY_GROUP_NAME
                                                             << ", sm_name=" << interpreter);
    return nullptr;
  }

  switch (static_cast<InterpreterType>(type))
  {
    case InterpreterType::LIDAR:
      return this->GetLidarProxyName(interpreterProxy, "LidarReader", "LidarStream");
    case InterpreterType::LIDAR_AND_POSE:
      return this->GetLidarProxyName(interpreterProxy, "LidarPoseReader", "LidarPoseStream");
    default:
      vtkErrorMacro("Could not find a proxy with this interpreter type.");
      return nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSMInterpretersManagerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
