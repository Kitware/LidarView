/*=========================================================================

  Program: LidarView
  Module:  lqHelper.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqHelper_h
#define lqHelper_h

#include <pqApplicationCore.h>
#include <pqDeleteReaction.h>
#include <pqPipelineSource.h>
#include <pqServerManagerModel.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSmartPointer.h>

#include "vtkLidarPoseReader.h"
#include "vtkLidarPoseStream.h"
#include "vtkLidarReader.h"
#include "vtkLidarStream.h"

#include <vector>

#include <QDialog>
#include <QSet>

/* This file contains some helpers function to manage proxy (stream and reader)
 * If you want to update some property of a proxy use
 * vtkSMPropertyHelper(proxy, "propertyName").Set(value);
 * and then to update your changes you have to use
 * vtkSMProxy->UpdateProperty("propertyName") or vtkSMProxy->UpdateVTKObjects();
 */

/**
 * @brief IsPositionOrientationStream return true if the src is a PositionOrientationStream
 * @param src to test
 * @return true if the src is a PositionOrientationStream
 */
bool IsPositionOrientationStream(pqPipelineSource* src);

/**
 * @brief IsLidarStream return true if the src is a LidarStream
 * @param src to test
 * @return true if the src is a LidarStream
 */
bool IsLidarStream(pqPipelineSource* src);

/**
 * @brief IsLidarReader return true if the src is a LidarReader
 * @param src to test
 * @return true if the src is a LidarReader
 */
bool IsLidarReader(pqPipelineSource* src);

/**
 * @brief IsLidarProxy return true if the proxy is a LidarReader or a LidarStream
 * @param proxy to test
 * @return true if the proxy is a LidarReader or a LidarStream
 */
bool IsLidarProxy(vtkSMProxy* proxy);

/**
 * @brief IsPositionOrientationProxy return true if the proxy is a
 *        PositionOrientationPacketReader or a PositionOrientationStream
 * @param proxy to test
 * @return true if the proxy is a PositionOrientationPacketReader or a PositionOrientationStream
 */
bool IsPositionOrientationProxy(vtkSMProxy* proxy);

/**
 * @brief IsStreamProxy return true if the proxy is a LidarStream or a PositionOrientationStream
 * @param proxy to test
 * @return true if the proxy is a LidarStream or a PositionOrientationStream
 */
bool IsStreamProxy(vtkSMProxy* proxy);

/**
 * @brief IsLidarReaderProxy return true if the proxy is a LidarReader
 * @param proxy to test
 * @return true if the proxy is a LidarReader
 */
bool IsLidarReaderProxy(vtkSMProxy* proxy);

/**
 * @brief IsLidarStreamProxy return true if the proxy is a LidarStream
 * @param proxy to test
 * @return true if the proxy is a LidarStream
 */
bool IsLidarStreamProxy(vtkSMProxy* proxy);

/**
 * @brief IsPositionOrientationReaderProxy return true if the proxy is a
 * PositionOrientationPacketReader
 * @param proxy to test
 * @return true if the proxy is a PositionOrientationPacketReader
 */
bool IsPositionOrientationReaderProxy(vtkSMProxy* proxy);

/**
 * @brief IsPositionOrientationStreamProxy return true if the proxy is a PositionOrientationStream
 * @param proxy to test
 * @return true if the proxy is a PositionOrientationStream
 */
bool IsPositionOrientationStreamProxy(vtkSMProxy* proxy);

/**
 * @brief hasLidarProxy look for a lidar proxy in the pipeline
 * @return true if there is a lidar proxy in the pipeline
 */
bool HasLidarProxy();

/**
 * @brief GetPipelineSourceFromProxy look for a lidar proxy in the pipeline
 * @return pqPipelineSource found
 */
pqPipelineSource* GetPipelineSourceFromProxy(vtkSMProxy* proxy);

/**
 * @brief SearchProxyByName search recursively a proxy in an other one using XMLName and XMLLabel
 * @param base_proxy
 * @param proxyName name of the proxy to search
 * @return the subproxy named 'proxyName', nullptr if nothing found
 */
vtkSMProxy* SearchProxyByName(vtkSMProxy* base_proxy, const std::string& proxyName);

/**
 * @brief SearchProxyByGroupName search recursively a proxy in an other one using the XMLGroup
 * @param base_proxy
 * @param proxyGroupName name of the XML group of the proxy to search
 * @return the subproxy of XML group 'proxyGroupName', nullptr if nothing found
 */
vtkSMProxy* SearchProxyByGroupName(vtkSMProxy* base_proxy, const std::string& proxyGroupName);

/**
 * @brief getLidarsProxy search lidar Proxy in the current pipeline
 * @return vector of all lidars proxys found
 */
std::vector<vtkSMProxy*> GetLidarsProxy();

/**
 * @brief GetPropertyFromProxy search a property in a proxy
 * @param proxy
 * @param propNameToFind name of the property to search
 * @return the property named 'propNameToFind', nullptr if nothing found
 */
vtkSMProperty* GetPropertyFromProxy(vtkSMProxy* proxy, const std::string& propNameToFind);

/**
 * @brief UpdateProxyProperty set the values to the property "propNameToFind" of the proxy
 *        If the property is not found in the proxy, a message is displayed but nothing is done.
 *        This function is useful, if the property type is unknown.
 *        If it is known, you should directly use vtkSMPropertyHelper to set the property
 * @param proxy proxy where to search the property
 * @param propNameToFind name of the property
 * @param values properties values to set
 * @return 1 if the property has been well set, 0 otherwise
 */
int UpdateProxyProperty(vtkSMProxy* proxy,
  const std::string& propNameToFind,
  const std::vector<std::string>& values);

/**
 * @brief GetGroupName Get the name of the first group where appear a proxy
 * @param existingProxy a proxy of the pipeline, use to get the ProxyDefinitionManager
 * @param proxyToFindName name of the proxy to look for
 * @return the name of the first group where a corresponding proxy is found
 */
std::string GetGroupName(vtkSMProxy* existingProxy, const std::string& proxyToFindName);

/**
 * @brief GetInterpreterTransform Get the interpreter transform of the proxy
 * @param proxy proxy of the pipeline, containing an interpreter proxy
 * @param translate will be filled by the function with the translation of the interpreter transform
 * @param rotate will be filled by the function with the rotation of the interpreter transform
 * @return true if the translate and the rotate vector have been well filled, false otherwise
 */
bool GetInterpreterTransform(vtkSMProxy* proxy,
  std::vector<double>& translate,
  std::vector<double>& rotate);

/**
 * @brief GetAllLinkedSources add recursively all consumers
 *        (sources that are directly linked to a sources) of originSource
 * @param originSource the source we want to list recursively all consumers
 * @param consumerListSources list where all consumers of originSource will be added
 */
void GetAllLinkedSources(pqPipelineSource* originSource,
  QSet<pqPipelineSource*>& consumerListSources);

/**
 * @brief GetFrameIndexOfTimestamp Get the Frame index of a given timestamp in the application
 * @param timestamp timestamp to get the number of frame
 * @return the frame index associated to timestamp in the application
 */
int GetFrameIndexOfTimestamp(double timestamp);

/**
 * @brief IsProxy Test if a vtkSMProxy can be cast into a type T
 * @param T cast type to test
 * @param proxy vtkSMProxy to test
 * @return true if the proxy can be cast in type T
 */
template <typename T>
bool IsProxy(vtkSMProxy* proxy)
{
  if (proxy != nullptr)
  {
    auto* tmp_objProxy = T::SafeDownCast(proxy->GetClientSideObject());
    if (tmp_objProxy)
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief GetProxies Get all vtkSMProxy that can be cast into a type T in the pipeline
 * @param T cast type to test
 * @return vector of all vtkSMProxies of the pipeline that can be cast in type T
 */
template <typename T>
std::vector<vtkSMProxy*> GetProxies()
{
  std::vector<vtkSMProxy*> proxys;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if (smmodel == nullptr)
  {
    return proxys;
  }
  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (IsProxy<T>(src->getProxy()))
    {
      proxys.push_back(src->getProxy());
    }
  }
  return proxys;
}

/**
 * @brief HasProxy Test if there is proxies that can be cast in T in the pipeline
 * @param T cast type to test
 * @return true if there is at least one proxy that can be cast into t in the pipeline
 */
template <typename T>
bool HasProxy()
{
  return !GetProxies<T>().empty();
}

/**
 * @brief RemoveAllProxyTypeFromPipelineBrowser
 *        Delete all sources of the pipeline that can be cast in T
 * @param T type of the sources to remove
 */
template <typename T>
void RemoveAllProxyTypeFromPipelineBrowser()
{
  // We remove all lidarReader and PositionOrientationReader in the pipeline
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if (smmodel == nullptr)
  {
    return;
  }

  // We reverse the pipeline to delete the last created first in the case of multi sensors
  QList<pqPipelineSource*> pipeline = smmodel->findItems<pqPipelineSource*>();
  std::reverse(std::begin(pipeline), std::end(pipeline));
  Q_FOREACH (pqPipelineSource* src, pipeline)
  {
    QSet<pqPipelineSource*> sources;
    if (IsProxy<T>(src->getProxy()))
    {
      GetAllLinkedSources(src, sources);
      sources.insert(src);
    }

    QSet<pqProxy*> sourcesProxy;
    Q_FOREACH (pqPipelineSource* src, sources)
    {
      sourcesProxy.insert(static_cast<pqProxy*>(src));
    }
    pqDeleteReaction::deleteSources(sourcesProxy);
  }
}

/**
 * @brief GetLidarProxyOfInterpreter
 *        Find the lidarProxy of type T (lidarReader or lidarStream)
 *        that have for interpreter specificInterpreter
 * @param proxyType type of the proxy to return
 * @param interpreterType type of the specificInterpreter
 * @return the VTKSMProxy of type proxyType that have for interpreter specificInterpreter
 */
//-----------------------------------------------------------------------------
template <typename proxyType, typename interpreterType>
vtkSMProxy* GetLidarProxyOfInterpreter(vtkSmartPointer<interpreterType> specificInterpreter)
{
  std::vector<vtkSMProxy*> lidarProxies = GetProxies<proxyType>();
  for (vtkSMProxy* lidarProxy : lidarProxies)
  {
    vtkSMProperty* interpProp = lidarProxy->GetProperty("PacketInterpreter");
    if (!interpProp)
    {
      continue;
    }

    vtkSMProxy* interpProxy = vtkSMPropertyHelper(interpProp).GetAsProxy();
    if (!interpProxy)
    {
      continue;
    }

    vtkSmartPointer<interpreterType> interp =
      interpreterType::SafeDownCast(interpProxy->GetClientSideObject());
    if (interp && interp == specificInterpreter)
    {
      return lidarProxy;
    }
  }
  return nullptr;
}

#endif // lqHelper_h
