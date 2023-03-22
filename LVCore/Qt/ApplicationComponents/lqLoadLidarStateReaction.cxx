#include "lqLoadLidarStateReaction.h"
#include "lqHelper.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <pqApplicationCore.h>
#include <pqSettings.h>

#include <vtkSMBooleanDomain.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSmartPointer.h>

#include <cstring>
#include <exception>
#include <iostream>
#include <vector>

//-----------------------------------------------------------------------------
lqLoadLidarStateReaction::lqLoadLidarStateReaction(QAction* action)
  : Superclass(action)
{
}

//-----------------------------------------------------------------------------
void lqLoadLidarStateReaction::onTriggered()
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if (smmodel == nullptr)
  {
    return;
  }

  // The first lidar source by default
  int index = 0;
  QStringList lidarNames;
  std::vector<vtkSMProxy*> lidarProxys;
  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (IsLidarProxy(src->getProxy()))
    {
      lidarProxys.push_back(src->getProxy());
      lidarNames << src->getSMName();
    }
  }

  if (lidarProxys.size() > 1)
  {
    lqChooseLidarDialog dialog(nullptr, lidarNames);
    if (!dialog.exec())
    {
      return;
    }
    index = dialog.getSelectedLidarIndex();
  }

  if (lidarProxys.size() < 1 || lidarProxys[index] == nullptr)
  {
    QMessageBox::warning(nullptr, tr(""), tr("No lidar source found in the pipeline"));
    return;
  }

  lqLoadLidarStateReaction::LoadLidarState(lidarProxys[index]);
}

//-----------------------------------------------------------------------------
void lqLoadLidarStateReaction::LoadLidarState(vtkSMProxy* lidarCurrentProxy)
{
  // Get the Lidar state file
  QString LidarStateFile = QFileDialog::getOpenFileName(nullptr,
    QString("Choose the lidar state file to load on the first lidar"),
    "",
    QString("json (*.json)"));

  if (LidarStateFile.isEmpty())
  {
    return;
  }
  // Set LidarState file directory to get back here whenever we load / save it
  if (!LidarStateFile.isNull() && !LidarStateFile.isEmpty())
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QFileInfo fileInfo(LidarStateFile);
    settings->setValue("LidarPlugin/OpenData/DefaultDirState", fileInfo.absolutePath());
  }

  // Read and get information of the JSON file
  Json::Value contents;
  std::ifstream file(LidarStateFile.toStdString());
  try
  {
    file >> contents;
  }
  catch (...)
  {
    QMessageBox::warning(nullptr, tr(""), tr("Json file not valid"));
    return;
  }

  // Read the Json file
  std::vector<propertyInfo> propertyInfo;
  try
  {
    ParseJsonContent(contents, "", propertyInfo);
  }
  catch (...)
  {
    QMessageBox::warning(nullptr, tr(""), tr("Error when parsing json information"));
    return;
  }

  lqLidarStateDialog dialog(nullptr, propertyInfo, "Please select the parameters to load");
  if (dialog.exec())
  {
    for (const auto& currentProp : dialog.properties)
    {
      if (currentProp.checkbox->isChecked())
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
          std::string message = "No matching proxy found. Property " + propertyName +
            " of the proxy " + proxyName + " not applied";
          QMessageBox::information(nullptr, tr(""), tr(message.c_str()));
        }
        else
        {
          UpdateProxyProperty(lidarProxy, propertyName, currentProp.values);
        }
      }
    }
    // Update the proxy
    lidarCurrentProxy->UpdateSelfAndAllInputs();
    vtkSMSourceProxy* sourcelidarProxy = vtkSMSourceProxy::SafeDownCast(lidarCurrentProxy);
    if (sourcelidarProxy)
    {
      sourcelidarProxy->UpdatePipelineInformation();
    }
    pqApplicationCore::instance()->render();
  }
}

//-----------------------------------------------------------------------------
void lqLoadLidarStateReaction::ParseJsonContent(Json::Value contents,
  std::string ObjectName,
  std::vector<propertyInfo>& propertiesInfo)
{
  for (auto it = contents.begin(); it != contents.end(); it++)
  {
    std::string currentKey = it.key().asString();

    // If the content is an object, it is write as a title
    if (contents[currentKey].isObject())
    {
      propertyInfo prop = propertyInfo(currentKey, currentKey);
      propertiesInfo.push_back(prop);
      ParseJsonContent(contents[currentKey], currentKey, propertiesInfo);
    }
    else
    {
      std::vector<std::string> values;
      if (contents[currentKey].isArray())
      {
        for (unsigned int j = 0; j < contents[currentKey].size(); j++)
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
