#include "lqHelper.h"

#include <QMessageBox>
#include <QString>
#include <QObject>

#include "vtkLidarReader.h"
#include "vtkLidarStream.h"
#include <vtkSMBooleanDomain.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMPropertyHelper.h>

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqServerManagerModel.h>

#include <cstring>
#include <iostream>

//-----------------------------------------------------------------------------
bool IsLidarProxy(vtkSMProxy * proxy)
{
  if(proxy != nullptr)
  {
    auto* tmp_stream = dynamic_cast<vtkLidarStream*> (proxy->GetClientSideObject());
    auto* tmp_reader = dynamic_cast<vtkLidarReader*> (proxy->GetClientSideObject());
    if (tmp_stream || tmp_reader)
    {
      return true;
    }
  }
  return false;
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

//-----------------------------------------------------------------------------
void UpdateProperty(vtkSMProxy * proxy, const std::string &propNameToFind,
                    const std::vector<std::string> &values)
{
  // If there is no value to set, the request is skipped
  if(values.empty())
  {
    std::string message = "Property " + propNameToFind + " couldn't be applied";
    QMessageBox::information(nullptr, QObject::tr(""), QObject::tr(message.c_str()) );
    return;
  }

  vtkSMProperty* prop = GetPropertyFromProxy(proxy, propNameToFind);

  if(!prop)
  {
    return;
  }

  std::vector<double> propertyAsDoubleArray = vtkSMPropertyHelper(prop).GetDoubleArray();
  if(propertyAsDoubleArray.size() > 1)
  {
    if(propertyAsDoubleArray.size() != values.size())
    {
      std::cout << "Values to applied and base property does not have the same size" << std::endl;
    }
    std::vector<double> d;
    for(unsigned int j = 0; j < values.size(); j++)
    {
      d.push_back(std::stod(values[j]));
    }
    vtkSMPropertyHelper(prop).Set(d.data(), d.size());
  }
  else
  {
    // If the property is a valid variant we display it to the user
    vtkVariant propertyAsVariant = vtkSMPropertyHelper(prop).GetAsVariant(0);
    if(propertyAsVariant.IsValid())
    {
      if(propertyAsVariant.IsInt())
      {
        vtkSMBooleanDomain * boolDomain = vtkSMBooleanDomain::SafeDownCast(prop->FindDomain("vtkSMBooleanDomain"));
        if(boolDomain)
        {
          int value = (values[0].compare("false") == 0) ? 0 : 1;
          vtkSMPropertyHelper(prop).Set(value);
        }
        else
        {
          vtkSMPropertyHelper(prop).Set(std::stoi(values[0]));
        }
      }
      else if(propertyAsVariant.IsNumeric())
      {
        vtkSMPropertyHelper(prop).Set(std::stof(values[0]));

      }
      else if(propertyAsVariant.IsString())
      {
        vtkSMPropertyHelper(prop).Set(values[0].c_str());
      }
    }
  }
}
