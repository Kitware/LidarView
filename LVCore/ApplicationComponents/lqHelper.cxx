#include "lqHelper.h"

#include <pqPVApplicationCore.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QRect>
#include <QString>
#include <QObject>

#include <vtkSMSessionProxyManager.h>
#include <vtkSMBooleanDomain.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyDefinitionManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkPVProxyDefinitionIterator.h>

#include <cstring>
#include <iostream>

//-----------------------------------------------------------------------------
bool IsPositionOrientationStream(pqPipelineSource* src)
{
  return ( src != nullptr && IsPositionOrientationStreamProxy(src->getProxy()));
}

//-----------------------------------------------------------------------------
bool IsLidarStream(pqPipelineSource* src)
{
  return ( src != nullptr && IsLidarStreamProxy(src->getProxy()));
}

//-----------------------------------------------------------------------------
bool IsLidarReader(pqPipelineSource* src)
{
  return ( src != nullptr && IsLidarReaderProxy(src->getProxy()));
}

//-----------------------------------------------------------------------------
bool IsStreamProxy(vtkSMProxy * proxy)
{
  return IsLidarStreamProxy(proxy) || IsPositionOrientationStreamProxy(proxy);
}

//-----------------------------------------------------------------------------
bool IsLidarProxy(vtkSMProxy * proxy)
{
  return IsLidarReaderProxy(proxy) || IsLidarStreamProxy(proxy);
}

//-----------------------------------------------------------------------------
bool IsPositionOrientationProxy(vtkSMProxy * proxy)
{
  return IsPositionOrientationReaderProxy(proxy) || IsPositionOrientationStreamProxy(proxy);
}


//-----------------------------------------------------------------------------
bool IsLidarReaderProxy(vtkSMProxy * proxy)
{
  return IsProxy<vtkLidarReader *>(proxy);
}

//-----------------------------------------------------------------------------
bool IsLidarStreamProxy(vtkSMProxy * proxy)
{
  return IsProxy<vtkLidarStream *>(proxy);
}

//-----------------------------------------------------------------------------
bool IsPositionOrientationReaderProxy(vtkSMProxy * proxy)
{
  return IsProxy<vtkPositionOrientationPacketReader *>(proxy);
}

//-----------------------------------------------------------------------------
bool IsPositionOrientationStreamProxy(vtkSMProxy * proxy)
{
  return IsProxy<vtkPositionOrientationStream *>(proxy);
}

//-----------------------------------------------------------------------------
bool HasLidarProxy()
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (IsLidarProxy(src->getProxy()))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
pqPipelineSource* GetPipelineSourceFromProxy(vtkSMProxy * proxy)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if(smmodel)
  {
    foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    {
      if(src->getProxy()->GetGlobalID() == proxy->GetGlobalID())
      {
         return src;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* SearchProxyByName(vtkSMProxy * base_proxy, const std::string &proxyName)
{
  if(std::strcmp(base_proxy->GetXMLName(), proxyName.c_str()) == 0 ||
     std::strcmp(base_proxy->GetXMLLabel(), proxyName.c_str()) == 0)
  {
    return base_proxy;
  }

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(base_proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    if(strcmp(prop->GetClassName(), "vtkSMProperty") == 0)
    {
      // If the property is a simple "vtkSMProperty" (ex: "Start" for Stream proxy)
      // "GetAsProxy" function will generate a warning
      continue;
    }

    // Search the proxy in the subProxy if its valid
    vtkSMProxy* propertyAsProxy = vtkSMPropertyHelper(prop).GetAsProxy();
    if(propertyAsProxy)
    {
      vtkSMProxy* subProxy = SearchProxyByName(propertyAsProxy, proxyName);
      if(subProxy)
      {
        return subProxy;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* SearchProxyByGroupName(vtkSMProxy * base_proxy, const std::string &proxyGroupName)
{
  if(std::strcmp(base_proxy->GetXMLGroup(), proxyGroupName.c_str()) == 0)
  {
    return base_proxy;
  }

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(base_proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    if(strcmp(prop->GetClassName(), "vtkSMProperty") == 0)
    {
      // If the property is a simple "vtkSMProperty" (ex: "Start" for Stream proxy)
      // "GetAsProxy" function will generate a warning
      continue;
    }

    // Search the proxy in the subProxy if its valid
    vtkSMProxy* propertyAsProxy = vtkSMPropertyHelper(prop).GetAsProxy();
    if(propertyAsProxy)
    {
      vtkSMProxy* subProxy = SearchProxyByGroupName(propertyAsProxy, proxyGroupName);
      if(subProxy)
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
  if(smmodel == nullptr)
  {
    return lidarProxys;
  }
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if(IsLidarProxy(src->getProxy()))
    {
      lidarProxys.push_back(src->getProxy());
    }
  }
  return lidarProxys;
}

//-----------------------------------------------------------------------------
vtkSMProperty* GetPropertyFromProxy(vtkSMProxy * proxy, const std::string &propNameToFind)
{
  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    const char* propName = propIter->GetKey();

    // If the name does not match we skip the property
    if(std::strcmp(propName, propNameToFind.c_str()) == 0)
    {
      return prop;
    }
  }
  std::cout << "Property not found " << std::endl;
  return nullptr;
}

std::string GetGroupName(vtkSMProxy * existingProxy, const std::string & proxyToFindName)
{
  vtkSMSessionProxyManager* pxm = existingProxy->GetSessionProxyManager();

  if(!pxm)
  {
    std::cout << "Couldn't get the SM Session Proxy Manager" << std::endl;
    return "";
  }

  vtkSMProxyDefinitionManager* pxdm = pxm->GetProxyDefinitionManager();

  if(!pxdm)
  {
    std::cout << "Couldn't get the SM Proxy Definition Manager" << std::endl;
    return "";
  }

  vtkPVProxyDefinitionIterator* iter = pxdm->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
     if(strcmp(iter->GetProxyName(), proxyToFindName.c_str()) == 0)
     {
       return iter->GetGroupName();
     }
  }
  return "";
}

//-----------------------------------------------------------------------------
bool GetInterpreterTransform(vtkSMProxy * proxy, std::vector<double>& translate,
                             std::vector<double>& rotate)
{
  vtkSMProperty * interpreterProp = proxy->GetProperty("PacketInterpreter");
  if(!interpreterProp)
  {
    return false;
  }

  vtkSMProxy * interpreterProxy = vtkSMPropertyHelper(interpreterProp).GetAsProxy();
  if(!interpreterProxy)
  {
    return false;
  }

  vtkSMProperty * transformProp = interpreterProxy->GetProperty("Sensor Transform");
  if(!transformProp)
  {
    return false;
  }

  vtkSMProxy * transormProxy = vtkSMPropertyHelper(transformProp).GetAsProxy();
  if(!transormProxy)
  {
    return false;
  }

  vtkSMProperty * translateProp = transormProxy->GetProperty("Position");
  vtkSMProperty * rotateProp = transormProxy->GetProperty("Rotation");
  if(!rotateProp || !translateProp)
  {
    return false;
  }

  translate = vtkSMPropertyHelper(translateProp).GetDoubleArray();
  rotate = vtkSMPropertyHelper(rotateProp).GetDoubleArray();

  return true;
}

//-----------------------------------------------------------------------------
void DisplayDialogOnActiveWindow(QDialog & dialog)
{
  // If there is multiple screen, we need to ensure that the dialog pop up on the active screen
  if (QApplication::desktop()->numScreens() > 1)
  {
    QRect oldGeometry = dialog.geometry();
    QRect availableScreenGeometry = QApplication::desktop()->availableGeometry(QApplication::activeWindow());

    // If the top left corner of the oldGeometry is outside of the active window screen geometry rectangle
    // We move the dialog to the new active window
    // Screen of the active window is defined by the center of the application window
    if(!(availableScreenGeometry.x() < oldGeometry.x() &&
       oldGeometry.x() < availableScreenGeometry.x() + availableScreenGeometry.width() &&
       availableScreenGeometry.y() < oldGeometry.y() &&
       oldGeometry.y() < availableScreenGeometry.y() + availableScreenGeometry.height()))
    {
      dialog.setGeometry(availableScreenGeometry.x() + 100,
                         availableScreenGeometry.y() + 100,
                         oldGeometry.width(), oldGeometry.height());
    }
  }
}

//-----------------------------------------------------------------------------
void GetAllLinkedSources(pqPipelineSource * originSource,
                         QSet<pqPipelineSource*>& consumerListSources)
{
  // Get all consumers: sources that are directly link to the originSource output
  QList<pqPipelineSource*> subSources = originSource->getAllConsumers();
  foreach (pqPipelineSource* src, subSources)
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
  while(allTimes.size() > 0 &&
        indexFrame != allTimes.size() -1 &&
        timestamp > allTimes[indexFrame] &&
        timestamp >= ((allTimes[indexFrame] + allTimes[indexFrame+1]) / 2) )
  {
    indexFrame++;
  }
  return indexFrame;
}
