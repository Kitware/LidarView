/*=========================================================================

  Program: LidarView
  Module:  lqRecentlyUsedPcapLoader.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqRecentlyUsedPcapLoader.h"

#include <pqApplicationCore.h>
#include <pqFileDialogModel.h>
#include <pqLoadDataReaction.h>
#include <pqLoadStateReaction.h>
#include <pqRecentlyUsedResourcesList.h>
#include <pqServer.h>
#include <pqServerConfiguration.h>
#include <pqServerResource.h>
#include <vtkPVSession.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSettings.h>
#include <vtk_jsoncpp.h>

#include "lqOpenLidarReaction.h"

#include <QFileInfo>
#include <QtDebug>

#include <algorithm>
#include <cassert>

namespace
{
constexpr char SEPARATOR = '|';
}

//-----------------------------------------------------------------------------
lqRecentlyUsedPcapLoader::lqRecentlyUsedPcapLoader(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
lqRecentlyUsedPcapLoader::~lqRecentlyUsedPcapLoader() = default;

//-----------------------------------------------------------------------------
bool lqRecentlyUsedPcapLoader::canLoad(const pqServerResource& resource)
{
  return (resource.hasData("LIDARVIEW_PCAP_DATA") && resource.hasData("group") &&
    resource.hasData("name") && resource.hasData("settings"));
}

//-----------------------------------------------------------------------------
bool lqRecentlyUsedPcapLoader::load(const pqServerResource& resource, pqServer* server)
{
  assert(this->canLoad(resource));
  if (resource.hasData("LIDARVIEW_PCAP_DATA"))
  {
    return this->loadPcapData(resource, server);
  }
  return false;
}

//-----------------------------------------------------------------------------
QIcon lqRecentlyUsedPcapLoader::icon(const pqServerResource& resource)
{
  if (resource.hasData("LIDARVIEW_PCAP_DATA"))
  {
    return QIcon(":/lqResources/Icons/lqPcapFile.svg");
  }
  return QIcon();
}

//-----------------------------------------------------------------------------
QString lqRecentlyUsedPcapLoader::label(const pqServerResource& resource)
{
  return resource.path();
}

//-----------------------------------------------------------------------------
bool lqRecentlyUsedPcapLoader::loadPcapData(const pqServerResource& resource, pqServer* server)
{
  QString filename = resource.path();
  QString proxyGroup = resource.data("group");
  QString proxyName = resource.data("name");

  // Deserialize json settings from pqServerResource
  std::string settings = resource.data("settings").toStdString();
  std::replace(settings.begin(), settings.end(), ::SEPARATOR, ':');
  std::stringstream sstr(settings);
  Json::Value jsonValue;
  sstr >> jsonValue;

  // Create a temporary proxy to store settings
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy(qPrintable(proxyGroup), qPrintable(proxyName)));
  vtkSMSettings::DeserializeFromJSON(proxy, jsonValue);

  if (filename.isEmpty())
  {
    qCritical() << "Empty file name specified!";
    return false;
  }
  lqOpenLidarReaction::openLidarPcap(filename, server, proxy);

  // Always return false, so config can be changed otherwise, it keep old configuration.
  // In our case addPcapFileToRecentResources is re-called if the loading succeed
  return false;
}

//-----------------------------------------------------------------------------
bool lqRecentlyUsedPcapLoader::addPcapFileToRecentResources(pqServer* server,
  const QString& file,
  vtkSMProxy* proxy)
{
  if (server)
  {
    // Needed to get the display resource in case of port forwarding
    pqServerResource resource = server->getResource();
    pqServerConfiguration config = resource.configuration();
    if (!config.isNameDefault())
    {
      resource = config.resource();
    }

    // Serialize settings as json, ':' is replaced with a custom separator
    // as it is already used by pqServerResource to parse URI.
    auto value = vtkSMSettings::SerializeAsJSON(proxy);
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string str = Json::writeString(builder, value);
    std::replace(str.begin(), str.end(), ':', ::SEPARATOR);

    resource.setPath(file);
    resource.addData("LIDARVIEW_PCAP_DATA", "1");
    resource.addData("group", proxy->GetXMLGroup());
    resource.addData("name", proxy->GetXMLName());
    resource.addData("settings", str.c_str());

    pqApplicationCore* core = pqApplicationCore::instance();
    core->recentlyUsedResources().add(resource);
    core->recentlyUsedResources().save(*core->settings());
    return true;
  }
  return false;
}
