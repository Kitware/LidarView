/*=========================================================================

  Program:   LidarView
  Module:    lqWelcomeDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqWelcomeDialog.h"
#include "ui_lqWelcomeDialog.h"

#include <vtkSMPropertyHelper.h>
#include <vtkSMSessionProxyManager.h>

#include <QComboBox>

#include <pqApplicationCore.h>
#include <pqServer.h>
#include <pqSettings.h>

//-----------------------------------------------------------------------------
lqWelcomeDialog::lqWelcomeDialog(QWidget* Parent)
  : QDialog(Parent)
  , Ui(new Ui::lqWelcomeDialog())
{
  auto& ui = this->Ui;
  ui->setupUi(this);
  this->setObjectName("lqWelcomeDialog");
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  modeLabelList[lqLidarViewManager::LIDAR_VIEWER] = ui->labelLidarViewer;
  modeLabelList[lqLidarViewManager::POINT_CLOUD_TOOL] = ui->labelPointCloudTool;
  modeLabelList[lqLidarViewManager::ADVANCED_MODE] = ui->labelAdvancedMode;
  for (auto&& label : this->modeLabelList)
  {
    label->setVisible(false);
    label->setStyleSheet("background-color: white;");
  }
  ui->labelAboutImage->setStyleSheet("background-color: white;");

  ui->comboBoxMode->currentIndexChanged(this->currentMode);
  this->currentModeChanged(this->currentMode);
  QObject::connect(
    ui->comboBoxMode, SIGNAL(currentIndexChanged(int)), this, SLOT(currentModeChanged(int)));
  this->onDoNotShowAgainStateChanged(ui->DoNotShowAgainButton->checkState());
  QObject::connect(ui->DoNotShowAgainButton,
    SIGNAL(stateChanged(int)),
    this,
    SLOT(onDoNotShowAgainStateChanged(int)));
}

//-----------------------------------------------------------------------------
lqWelcomeDialog::~lqWelcomeDialog()
{
  delete this->Ui;
}

//-------------------------------------------------------------------------------------------
void lqWelcomeDialog::currentModeChanged(int modeIdx)
{
  this->modeLabelList[this->currentMode]->setVisible(false);
  this->modeLabelList[modeIdx]->setVisible(true);
  this->currentMode = static_cast<lqLidarViewManager::InterfaceModes>(modeIdx);

  lqLidarViewManager::instance()->updateInterfaceLayout(this->currentMode);
}

//-----------------------------------------------------------------------------
void lqWelcomeDialog::onDoNotShowAgainStateChanged(int state)
{
  bool showDialog = (state != Qt::Checked);

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("GeneralSettings.ShowWelcomeDialog", showDialog ? 1 : 0);

  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (!server)
  {
    qCritical("No active server available!");
    return;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  if (!pxm)
  {
    qCritical("No proxy manager!");
    return;
  }

  vtkSMProxy* proxy = pxm->GetProxy("settings", "GeneralSettings");
  if (proxy)
  {
    vtkSMPropertyHelper(proxy, "ShowWelcomeDialog").Set(showDialog ? 1 : 0);
  }
}
