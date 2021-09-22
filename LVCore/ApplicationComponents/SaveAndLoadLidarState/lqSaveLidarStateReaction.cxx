#include "lqSaveLidarStateReaction.h"
#include "lqHelper.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>

#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMBooleanDomain.h>

#include <vector>

//-----------------------------------------------------------------------------
lqSaveLidarStateReaction::lqSaveLidarStateReaction(QAction *action)
  : Superclass(action)
{
}

//-----------------------------------------------------------------------------
void lqSaveLidarStateReaction::onTriggered()
{
  // Get the first lidar source  
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if(smmodel == nullptr)
  {
    return;
  }
  vtkSMProxy * lidarProxy = nullptr;
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if(IsLidarProxy(src->getProxy()))
    {
      if(lidarProxy == nullptr)
      {
        lidarProxy = src->getProxy();
      }
      else
      {
        QMessageBox::warning(nullptr, tr(""), tr("Multiple lidars sources found, only the first one will be saved") );
      }
    }
  }

  if(lidarProxy == nullptr)
  {
    QMessageBox::warning(nullptr, tr(""), tr("No lidar source found in the pipeline") );
    return;
  }

  lqSaveLidarStateReaction::SaveLidarState(lidarProxy);

}

//-----------------------------------------------------------------------------
void lqSaveLidarStateReaction::SaveLidarState(vtkSMProxy * lidarProxy)
{
  // Save Lidar Save information file
  QString defaultFileName = QString("SaveLidarInformation.json");
  QString StateFile = QFileDialog::getSaveFileName(nullptr,
                                 QString("File to save the first lidar information:"),
                                 defaultFileName, QString("json (*.json)"));

  if(StateFile.isEmpty())
  {
    return;
  }
  std::vector<propertyInfo> propertiesInfo;
  constructPropertiesInfo(lidarProxy, propertiesInfo);

  lqLidarStateDialog dialog(nullptr, propertiesInfo, "Please select the parameters to save");
  if(dialog.exec())
  {
    Json::StreamWriterBuilder builder;
    const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ofstream configFile(StateFile.toStdString());

    Json::Value data;

    for(unsigned int i = 0; i < dialog.properties.size(); i++)
    {
      propertyInfo currentProperty = dialog.properties[i];

      if(currentProperty.isProxy())
      {
        continue;
      }

      if(currentProperty.checkbox->isChecked())
      {
        std::string proxyName = currentProperty.proxyName;
        std::string propertyName = currentProperty.propertyName;
        std::vector<std::string> values = currentProperty.values;

        Json::Value vec(Json::arrayValue);
        if(values.size() == 1)
        {
          data[proxyName][propertyName] = values[0];
        }
        else
        {
          for(unsigned int i = 0; i < values.size(); i++)
          {
            vec.append(Json::Value(values[i]));
          }
          data[proxyName][propertyName] = vec;
        }
      }
    }
    if(data.empty())
    {
      QMessageBox::information(nullptr, QObject::tr(""), QObject::tr("Saved json file is empty (no parameter selected)"));
    }
    writer->write(data, &configFile);
  }

}

//-----------------------------------------------------------------------------
void lqSaveLidarStateReaction::constructPropertiesInfo(vtkSMProxy * lidarProxy,
                                                       std::vector<propertyInfo>& propertiesVector)
{
  std::vector<vtkSMProxy*> proxysToCompute;

  std::string proxyName = lidarProxy->GetXMLName();

  // Push the proxyName into the properties vector
  propertyInfo proxy = propertyInfo(proxyName, proxyName);
  propertiesVector.push_back(proxy);

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(lidarProxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMProperty* prop = propIter->GetProperty();
    const char* propertyName = propIter->GetKey();

    // Do not let the user check property that are in read only
    if(prop->GetInformationOnly())
    {
      continue;
    }

    // Do not handle repeatable property
    if(prop->GetRepeatable())
    {
      continue;
    }

    if(strcmp(prop->GetClassName(), "vtkSMProperty") == 0)
    {
      // If the property is a simple "vtkSMProperty" (ex: "Start" for Stream proxy)
      // "GetAsProxy" function will generate a warning
      continue;
    }

    // If the property is a valid proxy, we print all the properties of the proxy at the end
    vtkSMProxy* propertyAsProxy = vtkSMPropertyHelper(prop).GetAsProxy();
    if(propertyAsProxy)
    {
      proxysToCompute.push_back(propertyAsProxy);
      continue;
    }

    std::vector<std::string> values = getValueOfPropAsString(prop);
    propertyInfo proxy = propertyInfo(proxyName, propertyName, values);
    propertiesVector.push_back(proxy);
  }

  // Print all properties contains in the proxy properties
  // We call the recursive function at the end
  // because the dialog will take the properties in the saved order
  // We want all specific properties of a proxy stick together
  for(unsigned int p = 0; p < proxysToCompute.size(); p++)
  {
    constructPropertiesInfo(proxysToCompute[p], propertiesVector);
  }
}

//-----------------------------------------------------------------------------
std::vector<std::string> lqSaveLidarStateReaction::getValueOfPropAsString(vtkSMProperty * prop)
{
  std::vector<std::string> result;

  // If the property is a valid double array we display all the element to the user
  std::vector<double> propertyAsDoubleArray = vtkSMPropertyHelper(prop).GetDoubleArray();
  if(propertyAsDoubleArray.size() > 1)
  {
    for(unsigned int j = 0; j < propertyAsDoubleArray.size(); j++)
    {
      result.push_back(std::to_string(propertyAsDoubleArray[j]));
    }
    return result;
  }
  else
  {
    // If the property is a valid variant we display it to the user
    vtkVariant propertyAsVariant = vtkSMPropertyHelper(prop).GetAsVariant(0);
    if(propertyAsVariant.IsValid())
    {
      // If the property belongs to the boolean domain we display True/False notation to the user
      vtkSMBooleanDomain * boolDomain = vtkSMBooleanDomain::SafeDownCast(prop->FindDomain("vtkSMBooleanDomain"));
      if(boolDomain)
      {
        std::string bool_to_string = (propertyAsVariant.ToInt() == 0) ? "false" : "true";
        result.push_back(bool_to_string);
        return result;
      }
      result.push_back(propertyAsVariant.ToString());
      return result;
    }
  }
  return result;
}
