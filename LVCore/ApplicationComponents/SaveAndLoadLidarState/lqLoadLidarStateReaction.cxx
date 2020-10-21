#include "lqLoadLidarStateReaction.h"
#include "lqHelper.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqServerManagerModel.h>

#include <vtkSmartPointer.h>
#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMBooleanDomain.h>

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
 // Get the Lidar state file
 QString LidarStateFile = QFileDialog::getOpenFileName(nullptr,
                                  QString("Choose the lidar state file to load on the first lidar"),
                                  "", QString("json (*.json)"));

  if(LidarStateFile.isEmpty())
  {
    return;
  }

  // Get the first lidar source
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if(smmodel == nullptr)
  {
    return;
  }
  vtkSMProxy * lidarCurrentProxy = nullptr;
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if(IsLidarProxy(src->getProxy()))
    {
      if(lidarCurrentProxy == nullptr)
      {
        lidarCurrentProxy = src->getProxy();
      }
      else
      {
        QMessageBox::warning(nullptr, tr(""), tr("Multiple lidars sources found, only the first one will be updated") );
      }
    }
  }

  if(lidarCurrentProxy == nullptr)
  {
    QMessageBox::warning(nullptr, tr(""), tr("No lidar source found in the pipeline") );
    return;
  }

  // Read and get information of the JSON file
  Json::Value contents;
  std::ifstream file(LidarStateFile.toStdString());
  try
  {
    file >> contents;
  }
  catch(std::exception e)
  {
    QMessageBox::warning(nullptr, tr(""), tr("Json file not valid") );
    return;
  }

  // Read the Json file
  std::vector<propertyInfo> propertyInfo;
  try
  {
    this->ParseJsonContent(contents, "",propertyInfo);
  }
  catch(std::exception e)
  {
    QMessageBox::warning(nullptr, tr(""), tr("Error when parsing json information") );
    return;
  }

  lqLidarStateDialog dialog(nullptr, propertyInfo);
  if(dialog.exec())
  {
    for(const auto & currentProp : dialog.properties)
    {
      if(currentProp.checkbox->isChecked())
      {
        std::string proxyName = currentProp.proxyName;
        std::string propertyName = currentProp.propertyName;
        vtkSMProxy* lidarProxy = this->SearchProxy(lidarCurrentProxy, proxyName);

        if (lidarProxy == nullptr)
        {
          std::string message = "No matching proxy found. Property " + propertyName + " of the proxy " + proxyName + " not applied";
          QMessageBox::information(nullptr, tr(""), tr(message.c_str()) );
        }
        else
        {
          this->UpdateProperty(lidarProxy, propertyName, currentProp.values);
        }
      }
    }
    //Update the proxy
    lidarCurrentProxy->UpdateSelfAndAllInputs();
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
vtkSMProxy* lqLoadLidarStateReaction::SearchProxy(vtkSMProxy * base_proxy,
                                                  std::string proxyName)
{
  if(std::strcmp(base_proxy->GetXMLName(), proxyName.c_str()) == 0)
  {
    return base_proxy;
  }

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(base_proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();

    // If the property is a valid proxy, we print all the properties of the proxy at the end
    vtkSMProxy* propertyAsProxy = vtkSMPropertyHelper(prop).GetAsProxy();
    if(propertyAsProxy)
    {
      return SearchProxy(propertyAsProxy, proxyName);
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void lqLoadLidarStateReaction::UpdateProperty(vtkSMProxy * proxy,
                                              std::string propNameToFind,
                                              std::vector<std::string> values)
{
  // If there is no value to set, the request is skipped
  if(values.size() < 1)
  {
    std::string message = "Property " + propNameToFind + " couldn't be applied";
    QMessageBox::information(nullptr, tr(""), tr(message.c_str()) );
    return;
  }

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    const char* propName = propIter->GetKey();

    // If the name does not match we skip the property
    if(std::strcmp(propName, propNameToFind.c_str()) != 0)
    {
      continue;
    }

    // Both properties match
    std::vector<double> propertyAsDoubleArray = vtkSMPropertyHelper(prop).GetDoubleArray();
    if(propertyAsDoubleArray.size() > 1)
    {
      if(propertyAsDoubleArray.size() != values.size())
      {
        std::cout << "Values to applied and base property does not have the same size" << std::endl;
      }
      double d[values.size()];
      for(unsigned int j = 0; j < values.size(); j++)
      {
        d[j] = std::stod(values[j]);
      }
      vtkSMPropertyHelper(prop).Set(d, values.size());
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
}
