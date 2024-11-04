/*=========================================================================

  Program: LidarView
  Module:  lqInterpreterWidget.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqInterpreterWidget.h"

#include <QCheckBox>
#include <QVBoxLayout>
#include <QWidget>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqProxyWidget.h>
#include <pqServer.h>
#include <pqSettings.h>

#include <vtkSMParaViewPipelineController.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSettings.h>

#include "lqLidarConfigurationDialog.h"

namespace
{
constexpr const char* FILENAME_PROPERTY = "FileName";
constexpr const char* IGNORED_PANEL_WIDGET[2] = { "command_button", "pause_livesource" };
constexpr const char* CHECKED_KEY = "GNSSCheckbox";

void HideProperty(vtkSMProperty* property)
{
  property->SetNoCustomDefault(1);
  property->SetIsInternal(true);
}
}

//-----------------------------------------------------------------------------
class lqInterpreterWidget::lqInternals
{
public:
  //-----------------------------------------------------------------------------
  pqProxyWidget* getCurrentWidget()
  {
    if (!this->GNSSCheckbox)
    {
      return this->LidarProxyWidget;
    }
    auto state = this->GNSSCheckbox->checkState();
    return this->getCurrentWidget(state);
  }

  //-----------------------------------------------------------------------------
  pqProxyWidget* getCurrentWidget(int state)
  {
    return state == Qt::Checked ? this->PoseProxyWidget : this->LidarProxyWidget;
  }

  //-----------------------------------------------------------------------------
  void savePoseState()
  {
    if (!this->GNSSCheckbox)
    {
      return;
    }

    pqSettings* settings = pqApplicationCore::instance()->settings();
    settings->beginGroup(lqLidarConfigurationDialog::SETTINGS_GROUP());
    settings->setValue(::CHECKED_KEY, this->GNSSCheckbox->isChecked());
    settings->endGroup();
  }

  //-----------------------------------------------------------------------------
  void loadPoseState()
  {
    if (!this->GNSSCheckbox)
    {
      return;
    }

    pqSettings* settings = pqApplicationCore::instance()->settings();
    settings->beginGroup(lqLidarConfigurationDialog::SETTINGS_GROUP());
    bool value = settings->value(::CHECKED_KEY, 0).toBool();
    settings->endGroup();
    this->GNSSCheckbox->setChecked(value);
  }

  //-----------------------------------------------------------------------------
  static bool trySetProxySettings(pqProxyWidget* widget, vtkSMProxy* targetProxy)
  {
    if (!widget || !widget->proxy())
    {
      return false;
    }
    vtkSMProxy* widgetProxy = widget->proxy();
    if (strcmp(widgetProxy->GetXMLGroup(), targetProxy->GetXMLGroup()) == 0 &&
      strcmp(widgetProxy->GetXMLName(), targetProxy->GetXMLName()) == 0)
    {
      widgetProxy->Copy(targetProxy);
      return true;
    }
    return false;
  }

  pqProxyWidget* LidarProxyWidget;
  pqProxyWidget* PoseProxyWidget;
  QCheckBox* GNSSCheckbox;
};

//-----------------------------------------------------------------------------
lqInterpreterWidget::lqInterpreterWidget(QWidget* parentW, const char* proxyName)
  : Superclass(parentW)
  , Internals(new lqInterpreterWidget::lqInternals())
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  this->Internals->LidarProxyWidget = lqInterpreterWidget::createProxyWidget(this, proxyName);
  layout->addWidget(this->Internals->LidarProxyWidget);

  this->enableAdvancedMode(false);
}

//-----------------------------------------------------------------------------
lqInterpreterWidget::lqInterpreterWidget(QWidget* parentW,
  const char* lidarProxyName,
  const char* poseProxyName)
  : Superclass(parentW)
  , Internals(new lqInterpreterWidget::lqInternals())
{
  auto& internals = this->Internals;

  QVBoxLayout* layout = new QVBoxLayout(this);
  internals->GNSSCheckbox = new QCheckBox("Enable GNSS interpretation", this);
  internals->LidarProxyWidget = lqInterpreterWidget::createProxyWidget(this, lidarProxyName);
  internals->PoseProxyWidget = lqInterpreterWidget::createProxyWidget(this, poseProxyName);

  internals->PoseProxyWidget->setVisible(false);
  QObject::connect(
    internals->GNSSCheckbox, &QCheckBox::stateChanged, this, &lqInterpreterWidget::changeGNSSState);
  internals->loadPoseState();

  layout->addWidget(internals->GNSSCheckbox);
  layout->addWidget(internals->LidarProxyWidget);
  layout->addWidget(internals->PoseProxyWidget);

  this->enableAdvancedMode(false);
}

//-----------------------------------------------------------------------------
lqInterpreterWidget::~lqInterpreterWidget() = default;

//-----------------------------------------------------------------------------
pqProxyWidget* lqInterpreterWidget::createProxyWidget(QWidget* parent, const char* proxyName)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("sources", proxyName));

  // Initialize sub-proxies
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy);

  // Set FileName property ignore default and internal (in case of Lidar Reader)
  // as we already choose filename in previous step
  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(proxy->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    if (strcmp(propIter->GetKey(), ::FILENAME_PROPERTY) == 0)
    {
      ::HideProperty(propIter->GetProperty());
    }
    const char* panelWidget = propIter->GetProperty()->GetPanelWidget();
    for (unsigned int idx = 0; idx < 2; idx++)
    {
      if (panelWidget && strcmp(panelWidget, ::IGNORED_PANEL_WIDGET[idx]) == 0)
      {
        ::HideProperty(propIter->GetProperty());
      }
    }
  }

  // Restore custom settings
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->GetProxySettings(proxy);

  pqProxyWidget* widgetProxy = new pqProxyWidget(proxy, parent);
  widgetProxy->setApplyChangesImmediately(true);
  return widgetProxy;
}

//-----------------------------------------------------------------------------
bool lqInterpreterWidget::trySetProxySettings(vtkSMProxy* targetProxy)
{
  if (!targetProxy)
  {
    return false;
  }

  auto& internals = this->Internals;
  if (lqInternals::trySetProxySettings(internals->LidarProxyWidget, targetProxy))
  {
    if (internals->GNSSCheckbox)
    {
      internals->GNSSCheckbox->setChecked(false);
    }
    return true;
  }
  if (lqInternals::trySetProxySettings(internals->PoseProxyWidget, targetProxy))
  {
    internals->GNSSCheckbox->setChecked(true);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void lqInterpreterWidget::changeGNSSState(int state)
{
  auto& internals = this->Internals;

  internals->LidarProxyWidget->setVisible(false);
  internals->PoseProxyWidget->setVisible(false);
  pqProxyWidget* selectedWidget = internals->getCurrentWidget(state);
  selectedWidget->setVisible(true);
  internals->savePoseState();
}

//-----------------------------------------------------------------------------
vtkSMProxy* lqInterpreterWidget::getProxy()
{
  return this->Internals->getCurrentWidget()->proxy();
}

//-----------------------------------------------------------------------------
void lqInterpreterWidget::enableAdvancedMode(bool enabled)
{
  auto& internals = this->Internals;
  internals->LidarProxyWidget->filterWidgets(enabled);
  if (internals->PoseProxyWidget)
  {
    internals->PoseProxyWidget->filterWidgets(enabled);
  }
}

//-----------------------------------------------------------------------------
void lqInterpreterWidget::saveAsDefaults()
{
  auto& internals = this->Internals;
  pqProxyWidget* selectedWidget = internals->getCurrentWidget();
  selectedWidget->saveAsDefaults();
}

//-----------------------------------------------------------------------------
void lqInterpreterWidget::restoreDefaults()
{
  auto& internals = this->Internals;
  pqProxyWidget* selectedWidget = internals->getCurrentWidget();
  selectedWidget->restoreDefaults();
}
