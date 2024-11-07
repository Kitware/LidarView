/*=========================================================================

  Program: LidarView
  Module:  lqLidarConfigurationDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLidarConfigurationDialog.h"
#include "ui_lqLidarConfigurationDialog.h"

#include <QComboBox>
#include <QDebug>
#include <QStyle>
#include <QToolButton>
#include <QWidget>

#include <vtkSMParaViewPipelineController.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSessionProxyManager.h>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqInterfaceTracker.h>
#include <pqServer.h>
#include <pqSettings.h>

#include "lqInterpreterWidget.h"
#include "vtkSMInterpretersManagerProxy.h"

#include <cstring>

namespace
{
bool DoesProxyExist(vtkSMSessionProxyManager* pxm, const char* proxyName)
{
  if (!pxm->HasDefinition("sources", proxyName))
  {
    qCritical() << proxyName << " was not found, please make sure it was declared correctly.";
    return false;
  }
  return true;
}
}

//-----------------------------------------------------------------------------
class lqLidarConfigurationDialog::lqInternals : public QObject
{
public:
  Ui::LidarConfigurationDialog Ui;

  //-------------------------------------------------------------------------------------------
  lqInternals(lqLidarConfigurationDialog* self)
  {
    Ui::LidarConfigurationDialog& UI = this->Ui;
    UI.setupUi(self);

    QObject::connect(self,
      &QDialog::finished,
      this,
      &lqLidarConfigurationDialog::lqInternals::saveDialogSettingsAsDefault);
    QObject::connect(self,
      &QDialog::accepted,
      this,
      &lqLidarConfigurationDialog::lqInternals::saveInterpreterAsDefault);

    QObject::connect(UI.InterpreterSelectionBox,
      QOverload<int>::of(&QComboBox::currentIndexChanged),
      this,
      &lqLidarConfigurationDialog::lqInternals::changeCurrentInterpreter);

    QObject::connect(UI.RestoreDefault,
      &QToolButton::clicked,
      this,
      &lqLidarConfigurationDialog::lqInternals::restoreInterpreterDefault);
    QObject::connect(UI.AdvancedButton,
      &QToolButton::toggled,
      this,
      &lqLidarConfigurationDialog::lqInternals::toggleInterpreterAdvancedMode);
  }

  //-----------------------------------------------------------------------------
  void restoreDefaultDialogSettings()
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    settings->beginGroup(lqLidarConfigurationDialog::SETTINGS_GROUP());
    auto selectionWidget = this->Ui.InterpreterSelectionBox;
    QString name = settings->value(selectionWidget->objectName(), "").toString();
    int index = selectionWidget->findText(name);
    if (index != -1)
    {
      selectionWidget->setCurrentIndex(index);
    }
    auto multisensorWidget = this->Ui.EnableMultiSensors;
    bool isChecked =
      settings->value(multisensorWidget->objectName(), multisensorWidget->isChecked()).toBool();
    multisensorWidget->setChecked(isChecked);
    settings->endGroup();
  }

  //-------------------------------------------------------------------------------------------
  bool hasAvailableInterpreter()
  {
    bool noInterpreter = this->Ui.StackedInterpreters->count() == 0;
    if (noInterpreter)
    {
      qCritical() << "No interpreter detected. Please load an interpreter plugin with a widget "
                     "interface, implemented with `add_lidarview_plugin_interpreter_widget` CMake "
                     "macro, to read or stream LiDARs data.";
    }
    return !noInterpreter;
  }

public:
  //-------------------------------------------------------------------------------------------
  void addInterpreterWidget(QWidget* widget)
  {
    this->Ui.StackedInterpreters->addWidget(widget);
    this->Ui.InterpreterSelectionBox->addItem(widget->objectName());
  }

  //-------------------------------------------------------------------------------------------
  lqInterpreterWidget* getCurrentInterpreter()
  {
    return qobject_cast<lqInterpreterWidget*>(this->Ui.StackedInterpreters->currentWidget());
  }

public Q_SLOTS:
  //-------------------------------------------------------------------------------------------
  void saveDialogSettingsAsDefault()
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    settings->beginGroup(lqLidarConfigurationDialog::SETTINGS_GROUP());
    auto selectionWidget = this->Ui.InterpreterSelectionBox;
    if (selectionWidget->count() != 0)
    {
      settings->setValue(selectionWidget->objectName(), selectionWidget->currentText());
    }
    auto multisensorWidget = this->Ui.EnableMultiSensors;
    settings->setValue(multisensorWidget->objectName(), multisensorWidget->isChecked());
    settings->endGroup();
  }

  //-------------------------------------------------------------------------------------------
  void changeCurrentInterpreter(int newIndex)
  {
    Ui::LidarConfigurationDialog& UI = this->Ui;
    if (UI.StackedInterpreters->count() == 0)
    {
      return;
    }

    // Disable advanced mode for previous widget to avoid widget size differences
    auto previousInterpreter = this->getCurrentInterpreter();
    if (previousInterpreter)
    {
      previousInterpreter->enableAdvancedMode(false);
    }
    UI.StackedInterpreters->adjustSize();

    Ui.StackedInterpreters->setCurrentIndex(newIndex);
    this->toggleInterpreterAdvancedMode();
  }

  //-------------------------------------------------------------------------------------------
  void saveInterpreterAsDefault()
  {
    auto interpreter = this->getCurrentInterpreter();
    interpreter->saveAsDefaults();
  }

  //-------------------------------------------------------------------------------------------
  void restoreInterpreterDefault()
  {
    auto interpreter = this->getCurrentInterpreter();
    interpreter->restoreDefaults();
  }

  //-------------------------------------------------------------------------------------------
  void toggleInterpreterAdvancedMode()
  {
    auto interpreter = this->getCurrentInterpreter();
    interpreter->enableAdvancedMode(this->Ui.AdvancedButton->isChecked());
  }
};

//-----------------------------------------------------------------------------
lqLidarConfigurationDialog::lqLidarConfigurationDialog(QWidget* parentObject,
  vtkSMInterpretersManagerProxy::Mode mode,
  vtkSMProxy* defaultProxy)
  : Superclass(parentObject)
  , Internals(new lqLidarConfigurationDialog::lqInternals(this))
{
  this->setObjectName("lqLidarConfigurationDialog");

  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkSMSessionProxyManager* pxm = server->proxyManager();

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("misc", "InterpretersManager"));
  vtkSMInterpretersManagerProxy* imProxy = vtkSMInterpretersManagerProxy::SafeDownCast(proxy);

  controller->PreInitializeProxy(imProxy);
  vtkSMPropertyHelper(imProxy, "Mode").Set(static_cast<vtkIdType>(mode));
  controller->PostInitializeProxy(imProxy);

  int defaultInterpreter = -1;
  auto interpreters = imProxy->GetAvailableInterpreters();
  for (auto& interpreter : interpreters)
  {
    const char* lidarProxyName =
      imProxy->GetLidarProxyName(interpreter, vtkSMInterpretersManagerProxy::LIDAR);
    if (!lidarProxyName || strlen(lidarProxyName) == 0)
    {
      continue;
    }
    const char* poseProxyName =
      imProxy->GetLidarProxyName(interpreter, vtkSMInterpretersManagerProxy::LIDAR_AND_POSE);
    bool hasPoseProxy = poseProxyName && strlen(poseProxyName) != 0;

    if (!::DoesProxyExist(pxm, lidarProxyName) &&
      (!hasPoseProxy || !::DoesProxyExist(pxm, poseProxyName)))
    {
      continue;
    }

    lqInterpreterWidget* widget;
    if (hasPoseProxy)
    {
      widget = new lqInterpreterWidget(this, lidarProxyName, poseProxyName);
    }
    else
    {
      widget = new lqInterpreterWidget(this, lidarProxyName);
    }
    widget->setObjectName(interpreter);
    this->Internals->addInterpreterWidget(widget);

    if (widget->trySetProxySettings(defaultProxy))
    {
      defaultInterpreter = this->Internals->Ui.InterpreterSelectionBox->count() - 1;
    }
  }
  this->Internals->changeCurrentInterpreter(0);
  this->Internals->restoreDefaultDialogSettings();

  if (defaultInterpreter != -1)
  {
    this->Internals->Ui.InterpreterSelectionBox->setCurrentIndex(defaultInterpreter);
  }
}

//-----------------------------------------------------------------------------
lqLidarConfigurationDialog::~lqLidarConfigurationDialog() = default;

//-----------------------------------------------------------------------------
bool lqLidarConfigurationDialog::isMultiSensorsEnabled()
{
  return this->Internals->Ui.EnableMultiSensors->isChecked();
}
//-----------------------------------------------------------------------------
vtkSMProxy* lqLidarConfigurationDialog::getProxy() const
{
  auto interpreter = this->Internals->getCurrentInterpreter();
  return interpreter->getProxy();
}

//-----------------------------------------------------------------------------
int lqLidarConfigurationDialog::exec()
{
  if (!this->Internals->hasAvailableInterpreter())
  {
    this->reject();
    return QDialog::Rejected;
  }
  return Superclass::exec();
}

//-----------------------------------------------------------------------------
void lqLidarConfigurationDialog::open()
{
  if (!this->Internals->hasAvailableInterpreter())
  {
    this->reject();
    return;
  }
  Superclass::open();
}
