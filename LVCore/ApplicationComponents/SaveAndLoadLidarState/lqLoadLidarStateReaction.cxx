#include "lqLoadLidarStateReaction.h"
#include "lqHelper.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <pqApplicationCore.h>

#include <vtkSMBooleanDomain.h>
#include <vtkSmartPointer.h>
#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>

#include <cstring>
#include <vector>
#include <iostream>
#include <exception>

//-----------------------------------------------------------------------------
lqLoadLidarStateReaction::lqLoadLidarStateReaction(QAction *action)
  : Superclass(action)
{
}

//-----------------------------------------------------------------------------
void lqLoadLidarStateReaction::onTriggered()
{
  // Get the first lidar source
  std::vector<vtkSMProxy*> lidarProxys = GetLidarsProxy();

  if(lidarProxys.empty())
  {
    QMessageBox::warning(nullptr, tr(""), tr("No lidar source found in the pipeline") );
    return;
  }

  if(lidarProxys.size() > 1)
  {
    QMessageBox::warning(nullptr, tr(""), tr("Multiple lidars sources found, only the first one will be updated") );
  }

  lqLoadLidarStateReaction::LoadLidarState(lidarProxys[0]);
}

//-----------------------------------------------------------------------------
void lqLoadLidarStateReaction::LoadLidarState(vtkSMProxy * lidarCurrentProxy)
{
  // Get the Lidar state file
  QString LidarStateFile = QFileDialog::getOpenFileName(nullptr,
                                 QString("Choose the lidar state file to load on the first lidar"),
                                 "", QString("json (*.json)"));

  if(LidarStateFile.isEmpty())
  {
    return;
  }

  // Read and get information of the JSON file
  Json::Value contents;
  std::ifstream file(LidarStateFile.toStdString());
  try
  {
    file >> contents;
  }
  catch(...)
  {
    QMessageBox::warning(nullptr, tr(""), tr("Json file not valid") );
    return;
  }

  // Read the Json file
  std::vector<propertyInfo> propertyInfo;
  try
  {
    ParseJsonContent(contents, "",propertyInfo);
  }
  catch(...)
  {
    QMessageBox::warning(nullptr, tr(""), tr("Error when parsing json information") );
    return;
  }

  lqLidarStateDialog dialog(nullptr, propertyInfo, "Please select the parameters to load");
  if(dialog.exec())
  {
    for(const auto & currentProp : dialog.properties)
    {
      if(currentProp.checkbox->isChecked())
      {
        std::string proxyName = currentProp.proxyName;
        std::string propertyName = currentProp.propertyName;
        vtkSMProxy* lidarProxy = SearchProxyByName(lidarCurrentProxy, proxyName);

        // If the proxy is not found, search proxy from the same proxygroup
        // ex : Apply a property from an other LidarPacketInterpreter to the current one
        if (lidarProxy == nullptr)
        {
          std::string proxyGroupName = GetGroupName(lidarCurrentProxy, proxyName);
          lidarProxy = SearchProxyByGroupName(lidarCurrentProxy, proxyGroupName);
        }

        if (lidarProxy == nullptr)
        {
          std::string message = "No matching proxy found. Property " + propertyName + " of the proxy " + proxyName + " not applied";
          QMessageBox::information(nullptr, tr(""), tr(message.c_str()) );
        }
        else
        {
          lqLoadLidarStateReaction::UpdateProxyProperty(lidarProxy, propertyName, currentProp.values);
        }
      }
    }
    //Update the proxy
    lidarCurrentProxy->UpdateSelfAndAllInputs();
    vtkSMSourceProxy * sourcelidarProxy = vtkSMSourceProxy::SafeDownCast(lidarCurrentProxy);
    if(sourcelidarProxy)
    {
      sourcelidarProxy->UpdatePipelineInformation();
    }
    pqApplicationCore::instance()->render();
  }
}

//-----------------------------------------------------------------------------
void lqLoadLidarStateReaction::ParseJsonContent(Json::Value contents, std::string ObjectName,
                                            std::vector<propertyInfo>& propertiesInfo)
{
  for(auto it = contents.begin(); it != contents.end(); it++)
  {
    std::string currentKey = it.key().asString();

    // If the content is an object, it is write as a title
    if(contents[currentKey].isObject())
    {
      propertyInfo prop = propertyInfo(currentKey, currentKey);
      propertiesInfo.push_back(prop);
      ParseJsonContent(contents[currentKey], currentKey, propertiesInfo);
    }
    else
    {
      std::vector<std::string> values;
      if(contents[currentKey].isArray())
      {
        for(unsigned int j = 0; j < contents[currentKey].size(); j++)
        {
          values.push_back(contents[currentKey][j].asString());
        }
      }
      // All data are saved as String so we don't need other case (Numeric, bool)
      else if (contents[currentKey].isString())
      {
        values.push_back(contents[currentKey].asString());
      }

      propertyInfo prop = propertyInfo(ObjectName, currentKey, values);
      propertiesInfo.push_back(prop);
    }
  }
}

//-----------------------------------------------------------------------------
int lqLoadLidarStateReaction::UpdateProxyProperty(vtkSMProxy * proxy, const std::string &propNameToFind,
                        const std::vector<std::string> &values)
{
  // If there is no value to set, the request is skipped
  if(values.empty())
  {
    std::string message = "Property " + propNameToFind + " couldn't be applied";
    QMessageBox::information(nullptr, QObject::tr(""), QObject::tr(message.c_str()) );
    return 0;
  }

  vtkSMProperty* prop = GetPropertyFromProxy(proxy, propNameToFind);

  if(!prop)
  {
    return 0;
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
    proxy->UpdateProperty(propNameToFind.c_str());
    return 1;
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
        if(boolDomain && (values[0].compare("false") == 0 || values[0].compare("true") == 0))
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
      proxy->UpdateProperty(propNameToFind.c_str());
      return 1;
    }
  }
  return 0;
}

