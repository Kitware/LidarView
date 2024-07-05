/*=========================================================================

  Program: LidarView
  Module:  lqStatusBar.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqStatusBar.h"

#include "lqSensorListWidget.h"
#include "vtkSMLidarProxy.h"
#include "vtkSMLidarStreamProxy.h"

#include <pqActiveObjects.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtksys/SystemTools.hxx>

#include <QFileInfo>
#include <QLabel>
#include <QString>

//-----------------------------------------------------------------------------
lqStatusBar::lqStatusBar(QWidget* parentObject)
  : Superclass(parentObject)
{
  this->filenameLabel = new QLabel(this);
  this->sensorInfoLabel = new QLabel(this);

  QObject::connect(&pqActiveObjects::instance(),
    &pqActiveObjects::sourceChanged,
    this,
    &lqStatusBar::onActiveSourceChanged);

  this->addWidget(this->filenameLabel);
  this->addWidget(this->sensorInfoLabel);
}

//-----------------------------------------------------------------------------
void lqStatusBar::onActiveSourceChanged(pqPipelineSource* activeSource)
{
  // Check if we have an active source
  if (!activeSource)
  {
    return;
  }
  // Get lidar source even with TrailingFrame filter
  pqPipelineSource* lidarSource = lqSensorListWidget::instance()->getActiveLidarSource();
  if (lidarSource)
  {
    activeSource = lidarSource;
  }

  if (vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(activeSource->getProxy()))
  {
    if (auto svp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("FileName")))
    {
      std::string filename = svp->GetElement(0);
      if (!filename.empty())
      {
        QFileInfo fileInfo(QString::fromStdString(filename));
        std::string message = "File: " + fileInfo.fileName().toStdString();
        this->filenameLabel->setText(message.c_str());
      }
    }
    else
    {
      this->filenameLabel->clear();
    }

    vtkSMLidarProxy* lidarProxy = vtkSMLidarProxy::SafeDownCast(proxy);
    if (lidarProxy && !lidarProxy->GetLidarInformation().empty())
    {
      std::string message = "Interpreter: " + lidarProxy->GetLidarInformation();
      this->sensorInfoLabel->setText(message.c_str());
    }
    else
    {
      this->sensorInfoLabel->clear();
    }
  }
}
