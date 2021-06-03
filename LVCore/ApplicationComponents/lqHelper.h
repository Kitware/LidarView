#ifndef LQHELPER_H
#define LQHELPER_H

#include <pqApplicationCore.h>
#include <pqDeleteReaction.h>
#include <pqPipelineSource.h>
#include <pqServerManagerModel.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vtkSmartPointer.h>
#include <vtkSMPropertyHelper.h>

#include "vtkLidarReader.h"
#include "vtkLidarStream.h"
#include "vtkPositionOrientationPacketReader.h"
#include "vtkPositionOrientationStream.h"

#include <vector>

#include<QDialog>

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
 * @brief IsLidarProxy return true if the proxy is a LidarReader or a LidarStream
 * @param proxy to test
 * @return true if the proxy is a LidarReader or a LidarStream
 */
bool IsLidarProxy(vtkSMProxy * proxy);

/**
 * @brief IsPositionOrientationProxy return true if the proxy is a
 *        PositionOrientationPacketReader or a PositionOrientationStream
 * @param proxy to test
 * @return true if the proxy is a PositionOrientationPacketReader or a PositionOrientationStream
 */
bool IsPositionOrientationProxy(vtkSMProxy * proxy);

/**
 * @brief IsStreamProxy return true if the proxy is a LidarStream or a PositionOrientationStream
 * @param proxy to test
 * @return true if the proxy is a LidarStream or a PositionOrientationStream
 */
bool IsStreamProxy(vtkSMProxy * proxy);

/**
 * @brief IsLidarReaderProxy return true if the proxy is a LidarReader
 * @param proxy to test
 * @return true if the proxy is a LidarReader
 */
bool IsLidarReaderProxy(vtkSMProxy * proxy);

/**
 * @brief IsLidarStreamProxy return true if the proxy is a LidarStream
 * @param proxy to test
 * @return true if the proxy is a LidarStream
 */
bool IsLidarStreamProxy(vtkSMProxy * proxy);

/**
 * @brief IsPositionOrientationReaderProxy return true if the proxy is a PositionOrientationPacketReader
 * @param proxy to test
 * @return true if the proxy is a PositionOrientationPacketReader
 */
bool IsPositionOrientationReaderProxy(vtkSMProxy * proxy);

/**
 * @brief IsPositionOrientationStreamProxy return true if the proxy is a PositionOrientationStream
 * @param proxy to test
 * @return true if the proxy is a PositionOrientationStream
 */
bool IsPositionOrientationStreamProxy(vtkSMProxy * proxy);

/**
 * @brief hasLidarProxy look for a lidar proxy in the pipeline
 * @return true if there is a lidar proxy in the pipeline
 */
bool HasLidarProxy();

/**
 * @brief GetPipelineSourceFromProxy look for a lidar proxy in the pipeline
 * @return pqPipelineSource found
 */
pqPipelineSource* GetPipelineSourceFromProxy(vtkSMProxy * proxy);

/**
 * @brief SearchProxyByName search recursively a proxy in an other one using XMLName and XMLLabel
 * @param base_proxy
 * @param proxyName name of the proxy to search
 * @return the subproxy named 'proxyName', nullptr if nothing found
 */
vtkSMProxy* SearchProxyByName(vtkSMProxy * base_proxy, const std::string & proxyName);

/**
 * @brief SearchProxyByGroupName search recursively a proxy in an other one using the XMLGroup
 * @param base_proxy
 * @param proxyGroupName name of the XML group of the proxy to search
 * @return the subproxy of XML group 'proxyGroupName', nullptr if nothing found
 */
vtkSMProxy* SearchProxyByGroupName(vtkSMProxy * base_proxy, const std::string & proxyGroupName);

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
vtkSMProperty* GetPropertyFromProxy(vtkSMProxy * proxy, const std::string &propNameToFind);

/**
 * @brief GetGroupName Get the name of the first group where appear a proxy
 * @param existingProxy a proxy of the pipeline, use to get the ProxyDefinitionManager
 * @param proxyToFindName name of the proxy to look for
 * @return the name of the first group where a corresponding proxy is found
 */
std::string GetGroupName(vtkSMProxy * existingProxy, const std::string & proxyToFindName);

/**
 * @brief GetInterpreterTransform Get the interpreter transform of the proxy
 * @param proxy proxy of the pipeline, containing an interpreter proxy
 * @param translate will be filled by the function with the translation of the interpreter transform
 * @param rotate will be filled by the function with the rotation of the interpreter transform

 */
void GetInterpreterTransform(vtkSMProxy * proxy, std::vector<double>& translate, std::vector<double>& rotate);

/**
 * @brief DisplayDialogOnActiveWindow Display the dialog on the active window
 * @param dialog dialog to display
 */
void DisplayDialogOnActiveWindow(QDialog & dialog);

/**
 * @brief GetAllLinkedSources add recursively all consumers
 *        (sources that are directly linked to a sources) of originSource
 * @param originSource the source we want to list recursively all consumers
 * @param consumerListSources list where all consumers of originSource will be added
 */
void GetAllLinkedSources(pqPipelineSource * originSource,
                         QSet<pqPipelineSource*>& consumerListSources);


template<typename T>
bool IsProxy(vtkSMProxy * proxy)
{
  if(proxy != nullptr)
  {
    auto* tmp_objProxy= dynamic_cast<T> (proxy->GetClientSideObject());
    if (tmp_objProxy)
    {
      return true;
    }
  }
  return false;
}

template<typename T>
void RemoveAllProxyTypeFromPipelineBrowser()
{
  // We remove all lidarReader and PositionOrientationReader in the pipeline
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if(smmodel == nullptr)
  {
    return;
  }
  QSet<pqPipelineSource*> sources;
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if(IsProxy<T>(src->getProxy()))
    {
      GetAllLinkedSources(src, sources);
      sources.insert(src);
    }
  }
  pqDeleteReaction::deleteSources(sources);
}

#endif // LQHELPER_H
