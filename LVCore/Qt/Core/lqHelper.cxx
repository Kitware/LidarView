/*=========================================================================

  Program: LidarView
  Module:  lqHelper.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqHelper.h"

#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqPVApplicationCore.h>

#include <QApplication>
#include <QGuiApplication>
#include <QMessageBox>
#include <QObject>
#include <QRect>
#include <QString>

#include <vtkPVProxyDefinitionIterator.h>
#include <vtkSMBooleanDomain.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMProxyDefinitionManager.h>
#include <vtkSMSessionProxyManager.h>

#include <cstring>
#include <iostream>

//-----------------------------------------------------------------------------
bool IsPositionOrientationStream(pqPipelineSource* src)
{
  return (src != nullptr && IsPositionOrientationStreamProxy(src->getProxy()));
}

//-----------------------------------------------------------------------------
bool IsLidarStream(pqPipelineSource* src)
{
  return (src != nullptr && IsLidarStreamProxy(src->getProxy()));
}

//-----------------------------------------------------------------------------
bool IsLidarReader(pqPipelineSource* src)
{
  return (src != nullptr && IsLidarReaderProxy(src->getProxy()));
}

//-----------------------------------------------------------------------------
bool IsStreamProxy(vtkSMProxy* proxy)
{
  return IsLidarStreamProxy(proxy) || IsPositionOrientationStreamProxy(proxy);
}

//-----------------------------------------------------------------------------
bool IsLidarProxy(vtkSMProxy* proxy)
{
  return IsLidarReaderProxy(proxy) || IsLidarStreamProxy(proxy);
}

//-----------------------------------------------------------------------------
bool IsPositionOrientationProxy(vtkSMProxy* proxy)
{
  return IsPositionOrientationReaderProxy(proxy) || IsPositionOrientationStreamProxy(proxy);
}

//-----------------------------------------------------------------------------
bool IsLidarReaderProxy(vtkSMProxy* proxy)
{
  return IsProxy<vtkLidarReader>(proxy);
}

//-----------------------------------------------------------------------------
bool IsLidarStreamProxy(vtkSMProxy* proxy)
{
  return IsProxy<vtkLidarStream>(proxy);
}

//-----------------------------------------------------------------------------
bool IsPositionOrientationReaderProxy(vtkSMProxy* proxy)
{
  return IsProxy<vtkLidarPoseReader>(proxy);
}

//-----------------------------------------------------------------------------
bool IsPositionOrientationStreamProxy(vtkSMProxy* proxy)
{
  return IsProxy<vtkLidarPoseStream>(proxy);
}

//-----------------------------------------------------------------------------
bool HasLidarProxy()
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (IsLidarProxy(src->getProxy()))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
pqPipelineSource* GetPipelineSourceFromProxy(vtkSMProxy* proxy)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if (smmodel)
  {
    Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    {
      if (src->getProxy()->GetGlobalID() == proxy->GetGlobalID())
      {
        return src;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* SearchProxyByName(vtkSMProxy* base_proxy, const std::string& proxyName)
{
  if (std::strcmp(base_proxy->GetXMLName(), proxyName.c_str()) == 0 ||
    std::strcmp(base_proxy->GetXMLLabel(), proxyName.c_str()) == 0)
  {
    return base_proxy;
  }

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(base_proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    if (strcmp(prop->GetClassName(), "vtkSMProperty") == 0)
    {
      // If the property is a simple "vtkSMProperty" (ex: "Start" for Stream proxy)
      // "GetAsProxy" function will generate a warning
      continue;
    }

    // Search the proxy in the subProxy if its valid
    vtkSMProxy* propertyAsProxy = vtkSMPropertyHelper(prop).GetAsProxy();
    if (propertyAsProxy)
    {
      vtkSMProxy* subProxy = SearchProxyByName(propertyAsProxy, proxyName);
      if (subProxy)
      {
        return subProxy;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* SearchProxyByGroupName(vtkSMProxy* base_proxy, const std::string& proxyGroupName)
{
  if (std::strcmp(base_proxy->GetXMLGroup(), proxyGroupName.c_str()) == 0)
  {
    return base_proxy;
  }

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(base_proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    if (strcmp(prop->GetClassName(), "vtkSMProperty") == 0)
    {
      // If the property is a simple "vtkSMProperty" (ex: "Start" for Stream proxy)
      // "GetAsProxy" function will generate a warning
      continue;
    }

    // Search the proxy in the subProxy if its valid
    vtkSMProxy* propertyAsProxy = vtkSMPropertyHelper(prop).GetAsProxy();
    if (propertyAsProxy)
    {
      vtkSMProxy* subProxy = SearchProxyByGroupName(propertyAsProxy, proxyGroupName);
      if (subProxy)
      {
        return subProxy;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
std::vector<vtkSMProxy*> GetLidarsProxy()
{
  std::vector<vtkSMProxy*> lidarProxys;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if (smmodel == nullptr)
  {
    return lidarProxys;
  }
  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (IsLidarProxy(src->getProxy()))
    {
      lidarProxys.push_back(src->getProxy());
    }
  }
  return lidarProxys;
}

//-----------------------------------------------------------------------------
vtkSMProperty* GetPropertyFromProxy(vtkSMProxy* proxy, const std::string& propNameToFind)
{
  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    const char* propName = propIter->GetKey();

    // If the name does not match we skip the property
    if (std::strcmp(propName, propNameToFind.c_str()) == 0)
    {
      return prop;
    }
  }
  std::cout << "Property not found " << std::endl;
  return nullptr;
}

//-----------------------------------------------------------------------------
int UpdateProxyProperty(vtkSMProxy* proxy,
  const std::string& propNameToFind,
  const std::vector<std::string>& values)
{
  // If there is no value to set, the request is skipped
  if (values.empty())
  {
    std::string message = "Property " + propNameToFind + " couldn't be applied";
    QMessageBox::information(nullptr, QObject::tr(""), QObject::tr(message.c_str()));
    return 0;
  }

  vtkSMProperty* prop = GetPropertyFromProxy(proxy, propNameToFind);

  if (!prop)
  {
    return 0;
  }

  std::vector<double> propertyAsDoubleArray = vtkSMPropertyHelper(prop).GetDoubleArray();
  if (propertyAsDoubleArray.size() > 1)
  {
    if (propertyAsDoubleArray.size() != values.size())
    {
      std::cout << "Values to applied and base property does not have the same size" << std::endl;
    }
    std::vector<double> d;
    for (unsigned int j = 0; j < values.size(); j++)
    {
      d.push_back(std::stod(values[j]));
    }
    vtkSMPropertyHelper(prop).Set(d.data(), d.size());
    proxy->UpdateProperty(propNameToFind.c_str());
    return 1;
  }

  // If the property is a valid variant we display it to the user
  vtkVariant propertyAsVariant = vtkSMPropertyHelper(prop).GetAsVariant(0);
  if (propertyAsVariant.IsValid())
  {
    if (propertyAsVariant.IsInt())
    {
      vtkSMBooleanDomain* boolDomain =
        vtkSMBooleanDomain::SafeDownCast(prop->FindDomain("vtkSMBooleanDomain"));
      if (boolDomain && (values[0].compare("false") == 0 || values[0].compare("true") == 0))
      {
        int value = (values[0].compare("false") == 0) ? 0 : 1;
        vtkSMPropertyHelper(prop).Set(value);
      }
      else
      {
        vtkSMPropertyHelper(prop).Set(std::stoi(values[0]));
      }
    }
    else if (propertyAsVariant.IsNumeric())
    {
      vtkSMPropertyHelper(prop).Set(std::stof(values[0]));
    }
    else if (propertyAsVariant.IsString())
    {
      vtkSMPropertyHelper(prop).Set(values[0].c_str());
    }
    proxy->UpdateProperty(propNameToFind.c_str());
    return 1;
  }
  return 0;
}

std::string GetGroupName(vtkSMProxy* existingProxy, const std::string& proxyToFindName)
{
  vtkSMSessionProxyManager* pxm = existingProxy->GetSessionProxyManager();

  if (!pxm)
  {
    std::cout << "Couldn't get the SM Session Proxy Manager" << std::endl;
    return "";
  }

  vtkSMProxyDefinitionManager* pxdm = pxm->GetProxyDefinitionManager();

  if (!pxdm)
  {
    std::cout << "Couldn't get the SM Proxy Definition Manager" << std::endl;
    return "";
  }

  vtkPVProxyDefinitionIterator* iter = pxdm->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (strcmp(iter->GetProxyName(), proxyToFindName.c_str()) == 0)
    {
      return iter->GetGroupName();
    }
  }
  return "";
}

//-----------------------------------------------------------------------------
bool GetInterpreterTransform(vtkSMProxy* proxy,
  std::vector<double>& translate,
  std::vector<double>& rotate)
{
  vtkSMProperty* interpreterProp = proxy->GetProperty("PacketInterpreter");
  if (!interpreterProp)
  {
    return false;
  }

  vtkSMProxy* interpreterProxy = vtkSMPropertyHelper(interpreterProp).GetAsProxy();
  if (!interpreterProxy)
  {
    return false;
  }

  vtkSMProperty* transformProp = interpreterProxy->GetProperty("Sensor Transform");
  if (!transformProp)
  {
    return false;
  }

  vtkSMProxy* transormProxy = vtkSMPropertyHelper(transformProp).GetAsProxy();
  if (!transormProxy)
  {
    return false;
  }

  vtkSMProperty* translateProp = transormProxy->GetProperty("Position");
  vtkSMProperty* rotateProp = transormProxy->GetProperty("Rotation");
  if (!rotateProp || !translateProp)
  {
    return false;
  }

  translate = vtkSMPropertyHelper(translateProp).GetDoubleArray();
  rotate = vtkSMPropertyHelper(rotateProp).GetDoubleArray();

  return true;
}

//-----------------------------------------------------------------------------
void GetAllLinkedSources(pqPipelineSource* originSource,
  QSet<pqPipelineSource*>& consumerListSources)
{
  // Get all consumers: sources that are directly link to the originSource output
  QList<pqPipelineSource*> subSources = originSource->getAllConsumers();
  Q_FOREACH (pqPipelineSource* src, subSources)
  {
    consumerListSources.insert(src);
    GetAllLinkedSources(src, consumerListSources);
  }
}

//-----------------------------------------------------------------------------
int GetFrameIndexOfTimestamp(double timestamp)
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene = mgr->getActiveScene();
  QList<double> allTimes = scene->getTimeSteps();

  // If the indexFrame is the last frame, we don't have to test anything else.
  // The condition depending of indexFrame+1 will never be tested in that case.
  // In LV, the frame number change when the middle between 2 timesteps is reached :
  // For example, if we have 3 frames:
  // frame 0 at time 1.12, frame 1 at time 1.22 and frame 2 at time 1.32
  // frame 0 is from time 1.12 to 1.17
  // frame 1 is from 1.18 to 1.27
  // frame 2 "starts" at time 1.28
  int indexFrame = 0;
  while (!allTimes.empty() && indexFrame != allTimes.size() - 1 &&
    timestamp > allTimes[indexFrame] &&
    timestamp >= ((allTimes[indexFrame] + allTimes[indexFrame + 1]) / 2))
  {
    indexFrame++;
  }
  return indexFrame;
}
