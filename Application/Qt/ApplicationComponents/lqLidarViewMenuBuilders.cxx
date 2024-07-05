/*=========================================================================

  Program: LidarView
  Module:  lqLidarViewMenuBuilders.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLidarViewMenuBuilders.h"

#include "ui_lqEditMenuBuilder.h"
#include "ui_lqFileMenuBuilder.h"
#include "ui_lqHelpMenuBuilder.h"

#include <vtkSMProxyManager.h>

#include <pqApplicationCore.h>
#include <pqAxesToolbar.h>
#include <pqCameraToolbar.h>
#include <pqCustomViewpointsToolbar.h>
#include <pqDeleteReaction.h>
#include <pqDesktopServicesReaction.h>
#include <pqParaViewMenuBuilders.h>
#include <pqPropertiesPanel.h>
#include <pqPythonManager.h>
#include <pqSaveDataReaction.h>
#include <pqSetName.h>

#include "vtkLVVersion.h"
#include "vtkSMInterpretersManagerProxy.h"

#include "lqAboutDialogReaction.h"
#include "lqColorToolbar.h"
#include "lqInterfaceControlsToolbar.h"
#include "lqMainControlsToolbar.h"
#include "lqMeasurementGridReaction.h"
#include "lqMenuSaveAsReaction.h"
#include "lqOpenLidarReaction.h"
#include "lqRulersToolbar.h"
#include "lqSavePcapReaction.h"
#include "lqSensorListWidget.h"
#include "lqUpdateConfigurationReaction.h"

#include <QLayout>
#include <QMainWindow>
#include <QMenu>
#include <QString>

//-----------------------------------------------------------------------------
void lqLidarViewMenuBuilders::buildFileMenu(QMenu& menu)
{
  QString objectName = menu.objectName();
  // Build first paraview menu
  pqParaViewMenuBuilders::buildFileMenu(menu);
  // Add in the same menu lidarview functionalities
  Ui::lqFileMenuBuilder ui;
  ui.setupUi(&menu);
  // Since the UI file tends to change the name of the menu.
  menu.setObjectName(objectName);

  new lqOpenLidarReaction(ui.actionOpenPcap, vtkSMInterpretersManagerProxy::Mode::READER);
  new lqOpenLidarReaction(ui.actionOpenSensorStream, vtkSMInterpretersManagerProxy::Mode::STREAM);
  new lqSavePcapReaction(ui.actionSavePcap);
  new pqSaveDataReaction(ui.actionSaveData);
  new lqMenuSaveAsReaction(ui.menuSaveAs);

  // Add fileOpen action from ParaView to menuOpen
  for (QAction* action : menu.actions())
  {
    if (action->objectName() == "actionFileOpen")
    {
      ui.menuOpen->addAction(action);
    }
  }
}

//-----------------------------------------------------------------------------
void lqLidarViewMenuBuilders::buildEditMenu(QMenu& menu, pqPropertiesPanel* propertiesPanel)
{
  QString objectName = menu.objectName();
  // Build first paraview menu
  pqParaViewMenuBuilders::buildEditMenu(menu, propertiesPanel);
  // Add in the same menu lidarview functionalities
  Ui::lqEditMenuBuilder ui;
  ui.setupUi(&menu);
  // Since the UI file tends to change the name of the menu.
  menu.setObjectName(objectName);

  new lqUpdateConfigurationReaction(ui.actionUpdateConfiguration);

  auto* gridButton = new lqMeasurementGridReaction(ui.actionShowMeasurementGrid);
  QObject::connect(
    &menu, &QMenu::aboutToShow, gridButton, &lqMeasurementGridReaction::onRefreshButton);

  if (propertiesPanel)
  {
    /// Hook delete to pqDeleteReaction in pipeline browser.
    QAction* tempDeleteAction = new QAction(propertiesPanel);
    pqDeleteReaction* deleteReaction = new pqDeleteReaction(tempDeleteAction);
    deleteReaction->connect(
      propertiesPanel, SIGNAL(deleteRequested(pqProxy*)), SLOT(deleteSource(pqProxy*)));
  }
}

//-----------------------------------------------------------------------------
void lqLidarViewMenuBuilders::buildHelpMenu(QMenu& menu)
{
  QString objectName = menu.objectName();
  Ui::lqHelpMenuBuilder ui;
  ui.setupUi(&menu);
  // Since the UI file tends to change the name of the menu.
  menu.setObjectName(objectName);

  new pqDesktopServicesReaction(
    QUrl("https://gitlab.kitware.com/keu-computervision/slam/-/blob/master/paraview_wrapping/"
         "Plugin/doc/How_to_SLAM_with_LidarView.md"),
    ui.actionHowToSLAM);
  new pqDesktopServicesReaction(
    QUrl("https://www.kitware.com/what-we-offer"), ui.actionProfessionalSupport);

  new pqDesktopServicesReaction(QUrl("https://lidarview.kitware.com/"), ui.actionWebSite);
  new pqDesktopServicesReaction(
    QUrl("https://gitlab.kitware.com/LidarView/lidarview/-/releases"), ui.actionReleaseNotes);
  new pqDesktopServicesReaction(
    QUrl("https://discourse.paraview.org/c/lidar/15"), ui.actionCommunitySupport);

  QString paraviewVersionString = QString("%1.%2.%3")
                                    .arg(vtkSMProxyManager::GetVersionMajor())
                                    .arg(vtkSMProxyManager::GetVersionMinor())
                                    .arg(vtkSMProxyManager::GetVersionPatch());

  // Remote ParaView Guide
  QString guideURL = QString("https://docs.paraview.org/en/v%1/").arg(paraviewVersionString);
  new pqDesktopServicesReaction(QUrl(guideURL), ui.actionParaViewGuide);
  QString classroomTutorialsURL =
    QString("https://docs.paraview.org/en/v%1/Tutorials/ClassroomTutorials/index.html")
      .arg(paraviewVersionString);
  new pqDesktopServicesReaction(QUrl(classroomTutorialsURL), ui.actionParaViewTutorial);
  new pqDesktopServicesReaction(QUrl("https://www.paraview.org/"), ui.actionParaViewWebSite);

  QString bugReportURL = QString("https://gitlab.kitware.com/LidarView/lidarview/-/issues/"
                                 "new?issue[title]=Bug&issue[description]="
                                 "OS: %1\nLidarView Version: %2\n\nDescription:\n")
                           .arg(QSysInfo::prettyProductName())
                           .arg(LIDARVIEW_VERSION_FULL);
  new pqDesktopServicesReaction(QUrl(bugReportURL), ui.actionBugReport);
  new lqAboutDialogReaction(ui.actionAboutLidarView);
}

//-----------------------------------------------------------------------------
void lqLidarViewMenuBuilders::buildToolbars(QMainWindow& mainWindow)
{
  QToolBar* mainToolBar = new lqMainControlsToolbar(&mainWindow)
    << pqSetName("mainControlsToolbar");
  mainToolBar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, mainToolBar);

  QToolBar* colorToolbar = new lqColorToolbar(&mainWindow) << pqSetName("colorToolBar");
  colorToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, colorToolbar);

  QToolBar* interfaceControlsToolbar = new lqInterfaceControlsToolbar(&mainWindow)
    << pqSetName("interfaceControlsToolbar");
  interfaceControlsToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, interfaceControlsToolbar);

  // Give the macros menu to the pqPythonMacroSupervisor
  pqPythonManager* manager =
    qobject_cast<pqPythonManager*>(pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  if (manager)
  {
    QToolBar* macrosToolbar =
      new QToolBar(QCoreApplication::translate("pqmacrosToolbar", "Macros Toolbars"), &mainWindow)
      << pqSetName("macrosToolbar");
    macrosToolbar->hide();
    manager->addWidgetForRunMacros(macrosToolbar);
    mainWindow.addToolBar(Qt::TopToolBarArea, macrosToolbar);
  }

  QToolBar* cameraToolbar = new pqCameraToolbar(&mainWindow) << pqSetName("cameraToolbar");
  cameraToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, cameraToolbar);

  QToolBar* axesToolbar = new pqAxesToolbar(&mainWindow) << pqSetName("axesToolbar");
  axesToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, axesToolbar);

  QToolBar* rulersToolbar = new lqRulersToolbar(&mainWindow) << pqSetName("rulersToolbar");
  rulersToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, rulersToolbar);

  QToolBar* customViewpointsToolbar = new pqCustomViewpointsToolbar(&mainWindow)
    << pqSetName("customViewpointsToolbar");
  customViewpointsToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, customViewpointsToolbar);
}
