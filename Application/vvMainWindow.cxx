// Copyright 2013 Velodyne Acoustics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "vvMainWindow.h"
#include "ui_vvMainWindow.h"

#include "lqDockableSpreadSheetReaction.h"
#include "lqEnableAdvancedArraysReaction.h"
#include "lqLiveSourceScalarColoringBehavior.h"
#include "lqLoadLidarStateReaction.h"
#include "lqOpenPcapReaction.h"
#include "lqOpenRecentFilesReaction.h"
#include "lqOpenSensorReaction.h"
#include "lqSaveLASReaction.h"
#include "lqSaveLidarFrameReaction.h"
#include "lqSaveLidarStateReaction.h"
#include "lqUpdateCalibrationReaction.h"
#include <lqCameraParallelProjectionReaction.h>
#include <lqLidarViewManager.h>
#include <lqSensorListWidget.h>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqAxesToolbar.h>
#include <pqCameraToolbar.h>
#include <pqDataRepresentation.h>
#include <pqDeleteReaction.h>
#include <pqDesktopServicesReaction.h>
#include <pqHelpReaction.h>
#include <pqInterfaceTracker.h>
#include <pqLoadDataReaction.h>
#include <pqMainWindowEventManager.h>
#include <pqObjectBuilder.h>
#include <pqOutputWidget.h>
#include <pqParaViewBehaviors.h>
#include <pqParaViewMenuBuilders.h>
#include <pqPythonManager.h>
#include <pqRenderView.h>
#include <pqRenderViewSelectionReaction.h>
#include <pqServer.h>
#include <pqSetName.h>
#include <pqSettings.h>
#include <pqTabbedMultiViewWidget.h>

#include <vtksys/SystemTools.hxx>

// LidarView is always Python Enabled
#include <pqPythonDebugLeaksView.h>
#include <pqPythonShell.h>
#include <pvpythonmodules.h>
typedef pqPythonDebugLeaksView DebugLeaksViewType;

#include <QDockWidget>
#include <QDragEnterEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QToolBar>
#include <QUrl>

#include <cassert>
#include <iostream>
#include <sstream>

#include "lqPlayerControlsToolbar.h"
#include "lqViewFrameActions.h"

//-----------------------------------------------------------------------------
class vvMainWindow::pqInternals : public Ui::vvMainWindow
{
public:
  pqInternals()
    : Ui::vvMainWindow()
  {
  }
};

//-----------------------------------------------------------------------------
vvMainWindow::vvMainWindow()
{
  // ParaView Init Start
  // DebugLeaksView MUST be instantiated first
  DebugLeaksViewType* leaksView = nullptr;
  if (vtksys::SystemTools::GetEnv("PV_DEBUG_LEAKS_VIEW"))
  {
    leaksView = new DebugLeaksViewType(this);
    leaksView->setWindowFlags(Qt::Window);
    leaksView->show();
  }

  // Connect to builtin server.
  pqServer* server =
    pqApplicationCore::instance()->getObjectBuilder()->createServer(pqServerResource("builtin:"));
  pqActiveObjects::instance().setActiveServer(server);

  // PVPython
  pvpythonmodules_load();

  // Internals
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  // this->Internals->outputWidgetDock->hide(); // This is default ParaView, we give the App choice
  // first this->Internals->pythonShellDock->hide();  // Kept for reference
  // this->Internals->materialEditorDock->hide();

  // LidarView Specific Manager
  new lqLidarViewManager(this);

  // Create pythonshell
  pqPythonShell* shell = new pqPythonShell(this);
  shell->setObjectName("pythonShell");
  shell->setFontSize(8);
  lqLidarViewManager::instance()->setPythonShell(shell);
  if (leaksView)
  {
    leaksView->setShell(shell);
  }
  // ParaView Init END

  // Setup ParaView  Base GUI
  this->setupPVGUI();

  // Setup LidarView Base GUI
  this->setupLVGUI();

  // LidarView custom GUI
  this->setupGUICustom();

  // LidarView Branding
  this->setBranding();

  // Create Main Render View
  lqLidarViewManager::instance()->createMainRenderView();

  // Schedule Python Init late otherwise startup is slow, WIP to investigate (related to window
  // creation timing)
  lqLidarViewManager::instance()->schedulePythonStartup();

  // Force Show App
  this->show();
  this->raise();
  this->activateWindow();

  new pqParaViewBehaviors(this, this);
}

//-----------------------------------------------------------------------------
vvMainWindow::~vvMainWindow()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void vvMainWindow::setupPVGUI()
{
  // Common Parts of the ParaViewMainWindow.cxx
  // Easily updatable and referenceable
  // Common elements to all LidarView-based Apps

  // PARAVIEW_USE_MATERIALEDITOR // Not used

  // show output widget if we received an error message.
  this->connect(this->Internals->outputWidget,
    SIGNAL(messageDisplayed(const QString&, int)),
    SLOT(handleMessage(const QString&, int)));

  // Setup default GUI layout.
  this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // CUSTOMIZED tabifyDockWidget() / hide() section
  // Easier to understand what is being used

  // Memory Inspector Dock
  this->tabifyDockWidget(this->Internals->informationDock, this->Internals->memoryInspectorDock);
  this->Internals->memoryInspectorDock->hide();

  // View Animation Dock
  this->tabifyDockWidget(this->Internals->informationDock, this->Internals->viewAnimationDock);
  this->Internals->viewAnimationDock->hide();

  // Hide Various Other Docks
  this->Internals->colorMapEditorDock->hide();
  this->Internals->pipelineBrowserDock->hide();
  this->Internals->propertiesDock->hide();
  this->Internals->viewPropertiesDock->hide();
  this->Internals->displayPropertiesDock->hide();
  this->Internals->informationDock->hide();

  // CUSTOMIZED PropertiesPanelMode
  this->Internals->propertiesPanel->setPanelMode(pqPropertiesPanel::SOURCE_PROPERTIES);
  this->Internals->viewPropertiesPanel->setPanelMode(pqPropertiesPanel::VIEW_PROPERTIES);
  this->Internals->displayPropertiesPanel->setPanelMode(pqPropertiesPanel::DISPLAY_PROPERTIES);

  // TODO update UI when font size changes.

  // TODO Enable help from the properties panel.

  /// hook delete to pqDeleteReaction.
  QAction* tempDeleteAction = new QAction(this);
  pqDeleteReaction* handler = new pqDeleteReaction(tempDeleteAction);
  handler->connect(this->Internals->propertiesPanel,
    SIGNAL(deleteRequested(pqProxy*)),
    SLOT(deleteSource(pqProxy*)));

  // setup color editor
  /// Provide access to the color-editor panel for the application.
  pqApplicationCore::instance()->registerManager(
    "COLOR_EDITOR_PANEL", this->Internals->colorMapEditorDock);

  // Set the central widget
  pqTabbedMultiViewWidget* mv = new pqTabbedMultiViewWidget;
  mv->setTabVisibility(false);
  this->setCentralWidget(mv);

  // Provide access to the find data panel for the application.
  // pqApplicationCore::instance()->registerManager("FIND_DATA_PANEL",
  // this->Internals->findDataDock); // Unused

  // Populate application menus with actions.
  // pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File); // Advanced Menu
  // pqParaViewMenuBuilders::buildEditMenu(*this->Internals->menu_Edit,
  // this->Internals->propertiesPanel); // Advanced Menu

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate extractors menu.
  // pqParaViewMenuBuilders::buildExtractorsMenu(*this->Internals->menuExtractors, this); // Unused

  // Populate Tools menu.
  // pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools); // Advanced Menu

  // Populate Catalyst menu.
  // pqParaViewMenuBuilders::buildCatalystMenu(*this->Internals->menu_Catalyst); // Unused

  // setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser->contextMenu());

  // CUSTOMIZED Toolbars
  // pqParaViewMenuBuilders::buildToolbars(*this);
  this->pqbuildToolbars();

  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menuViews, *this);

  // Setup the menu to show macros.
  // pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menu_Macros); // Advanced Menu

  // Setup the help menu.
  // pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help); // Custom

  // CUSTOMIZED Paraview behaviors
  pqParaViewBehaviors::enableApplyBehavior();
  pqParaViewBehaviors::enableAutoLoadPluginXMLBehavior();
  pqParaViewBehaviors::enableCommandLineOptionsBehavior();
  pqParaViewBehaviors::enableCrashRecoveryBehavior();
  pqParaViewBehaviors::enableDataTimeStepBehavior();
  pqParaViewBehaviors::enableLiveSourceBehavior();
  pqParaViewBehaviors::enableObjectPickingBehavior();
  pqParaViewBehaviors::enableQuickLaunchShortcuts();
  pqParaViewBehaviors::enableSpreadSheetVisibilityBehavior();
  pqParaViewBehaviors::enableStandardPropertyWidgets();
  pqParaViewBehaviors::setEnableStandardViewFrameActions(false);
  pqParaViewBehaviors::setEnableDefaultViewBehavior(false);
  pqParaViewBehaviors::setEnableUsageLoggingBehavior(true);
}

//-----------------------------------------------------------------------------
void vvMainWindow::pqbuildToolbars()
{
  // Rework of pqParaViewMenuBuilders::buildToolbars
  // Removed pqMainControlsToolbar, pqVCRToolbar, pqAnimationTimeToolbar,
  //         pqCustomViewpointsToolbar, pqColorToolbar, pqRepresentationToolbar

  QToolBar* cameraToolbar = new pqCameraToolbar(this) << pqSetName("cameraToolbar");
  this->addToolBar(Qt::TopToolBarArea, cameraToolbar);

  QToolBar* axesToolbar = new pqAxesToolbar(this) << pqSetName("axesToolbar");
  this->addToolBar(Qt::TopToolBarArea, axesToolbar);

  // Give the macros menu to the pqPythonMacroSupervisor
  pqPythonManager* manager =
    qobject_cast<pqPythonManager*>(pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  if (manager)
  {
    QToolBar* macrosToolbar = new QToolBar("Macros Toolbars", this) << pqSetName("MacrosToolbar");
    macrosToolbar->setVisible(false);
    manager->addWidgetForRunMacros(macrosToolbar);
    this->addToolBar(Qt::TopToolBarArea, macrosToolbar);
  }
}

//-----------------------------------------------------------------------------
void vvMainWindow::setupLVGUI()
{
  // Add generally common elements to all LidarView-based apps
  // Not necessarilly verbatim Paraview
  // Feel Free to modify this in your LidarView-based app if you really do not like the look

  // Output Window Dock
  this->Internals->outputWidgetDock->setWidget(this->Internals->outputWidget);
  this->tabifyDockWidget(this->Internals->informationDock, this->Internals->outputWidgetDock);
  connect(this->Internals->actionShowErrorDialog,
    SIGNAL(triggered()),
    this->Internals->outputWidgetDock,
    SLOT(show()));
  this->Internals->outputWidgetDock->hide();

  // Python Shell Dock
  this->Internals->pythonShellDock->setWidget(lqLidarViewManager::instance()->getPythonShell());
  this->tabifyDockWidget(this->Internals->informationDock, this->Internals->pythonShellDock);
  connect(this->Internals->actionPython_Console,
    SIGNAL(triggered()),
    this->Internals->pythonShellDock,
    SLOT(show()));
  this->Internals->pythonShellDock->hide();

  // Sensor List Dock //wipwip not a core LV element, could also be added in the .ui in internals,
  // like python shell
  QDockWidget* sensorListDock = new QDockWidget("Sensor List", this);
  sensorListDock->setObjectName("sensorListDock");
  sensorListDock->hide();
  sensorListDock->setWidget(lqSensorListWidget::instance());
  this->addDockWidget(Qt::RightDockWidgetArea, sensorListDock);
}

//-----------------------------------------------------------------------------
void vvMainWindow::setupGUICustom()
{
  // LidarView Specific UI elements

  // WIP TODO actionResetDefaultSettings needs rework
  connect(this->Internals->actionResetDefaultSettings,
    SIGNAL(triggered()),
    lqLidarViewManager::instance(),
    SLOT(onResetDefaultSettings()));

  // Add Professional Support menu action
  new pqDesktopServicesReaction(
    QUrl("https://www.kitware.com/what-we-offer"), (this->Internals->actionHelpSupport));
  // How to SLAM menu action
  new pqDesktopServicesReaction(
    QUrl("https://gitlab.kitware.com/keu-computervision/slam/-/blob/master/paraview_wrapping/"
         "Plugin/doc/How_to_SLAM_with_LidarView.md"),
    (this->Internals->actionHelpSlam));

  // Enable help from the properties panel.
  QObject::connect(this->Internals->propertiesPanel,
    SIGNAL(helpRequested(const QString&, const QString&)),
    this,
    SLOT(showHelpForProxy(const QString&, const QString&)));

  // Break ToolBar Lines
  this->addToolBarBreak();

  // LidarView-Base Toolbars
  QToolBar* vcrToolbar = new lqPlayerControlsToolbar(this) << pqSetName("Player Control");
  this->addToolBar(Qt::TopToolBarArea, vcrToolbar);

  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();
  pgm->addInterface(new lqViewFrameActions);

  // Create RenderView dependant reactions
  new lqCameraParallelProjectionReaction(this->Internals->actionToggleProjection);
  // new pqRenderViewSelectionReaction(this->Internals->actionSelect_Visible_Points, nullptr, // WIP
  //     pqRenderViewSelectionReaction::SELECT_SURFACE_POINTS);
  new pqRenderViewSelectionReaction(this->Internals->actionSelect_Points,
    nullptr,
    pqRenderViewSelectionReaction::SELECT_FRUSTUM_POINTS);

  // LV Reactions
  new lqDockableSpreadSheetReaction(this->Internals->actionSpreadsheet, this);

  // WIP SETTINGS GRID
  pqSettings* const settings = pqApplicationCore::instance()->settings();
  const QVariant& gridVisible = settings->value("LidarPlugin/MeasurementGrid/Visibility", true);
  this->Internals->actionMeasurement_Grid->setChecked(gridVisible.toBool());
  connect(this->Internals->actionMeasurement_Grid,
    SIGNAL(toggled(bool)),
    lqLidarViewManager::instance(),
    SLOT(onMeasurementGrid(bool)));

  // Ruler Menu
  connect(this->Internals->actionMeasure, SIGNAL(triggered()), this, SLOT(toggleMVDecoration()));

  new lqOpenSensorReaction(this->Internals->actionOpen_Sensor_Stream);
  new lqOpenPcapReaction(this->Internals->actionOpenPcap);

  new lqUpdateCalibrationReaction(
    this->Internals->actionChoose_Calibration_File); // Requires lqSensorListWidget init
  // Following is required if intend to use the lqSensorListWidget
  lqSensorListWidget::instance()->setCalibrationFunction(
    &lqUpdateCalibrationReaction::UpdateExistingSource);

  new lqSaveLidarStateReaction(this->Internals->actionSaveLidarState);
  new lqLoadLidarStateReaction(this->Internals->actionLoadLidarState);

  new lqOpenRecentFilesReaction(
    this->Internals->menuRecent_Files, this->Internals->actionClear_Menu);

  // Writer reactions (action, writerName, extension, displaySettings, useDirectory,
  // keepNameFromPcapFile, fileNameWithFrameNumber )
  new lqSaveLidarFrameReaction(
    this->Internals->actionSavePCD, "PCDWriter", "pcd", false, false, true, true);
  new lqSaveLidarFrameReaction(
    this->Internals->actionSaveCSV, "DataSetCSVWriter", "csv", false, false, true, true);
  new lqSaveLidarFrameReaction(
    this->Internals->actionSavePLY, "PPLYWriter", "ply", false, false, true, true);
  new lqSaveLASReaction(this->Internals->actionSaveLAS, false, false, true, true);

  // Add save/load lidar state action
  new lqEnableAdvancedArraysReaction(this->Internals->actionEnableAdvancedArrays);

  // Stream AutoColoring
  new lqLiveSourceScalarColoringBehavior();

  // Advanced Menu
  // build Paraview file menu
  QMenu* paraviewFileMenu = this->Internals->menuAdvance->addMenu("File (Paraview)");
  pqParaViewMenuBuilders::buildFileMenu(*paraviewFileMenu);
  paraviewFileMenu->setTitle(
    "File (Paraview)"); // for some reason the menu builder rename the QMenu...

  // build Paraview edit menu
  QMenu* paraviewEditMenu = this->Internals->menuAdvance->addMenu("Edit (Paraview)");
  pqParaViewMenuBuilders::buildEditMenu(*paraviewEditMenu);
  paraviewEditMenu->setTitle(
    "Edit (Paraview)"); // for some reason the menu builder rename the QMenu...

  // build Paraview macro menu
  QMenu* paraviewMacroMenu = this->Internals->menuAdvance->addMenu("Macro (Paraview)");
  pqParaViewMenuBuilders::buildMacrosMenu(*paraviewMacroMenu);

  // build Paraview tools menu
  // MUST BE ISNTANTIATED LAST for qtTesting + requires pqTabbedMultiViewWidget
  QMenu* paraviewToolsMenu = this->Internals->menuAdvance->addMenu("Tools (Paraview)");
  pqParaViewMenuBuilders::buildToolsMenu(*paraviewToolsMenu);
  // DO NOT ADD ANYTHING BELOW
}

//-----------------------------------------------------------------------------
void vvMainWindow::setBranding()
{
// For good measure
#ifndef SOFTWARE_NAME
#error "SOFTWARE_NAME not defined"
#endif
  static_assert(SOFTWARE_NAME, "SOFTWARE_NAME is not defined");

  std::stringstream ss;
  ss << "Reset " << SOFTWARE_NAME << " settings";
  QString text = QString(ss.str().c_str());
  this->Internals->actionResetDefaultSettings->setText(text);
  ss.str("");
  ss.clear();

  ss << "This will reset all " << SOFTWARE_NAME << " settings by default";
  text = QString(ss.str().c_str());
  this->Internals->actionResetDefaultSettings->setIconText(text);
  ss.str("");
  ss.clear();

  ss << "About " << SOFTWARE_NAME;
  text = QString(ss.str().c_str());
  this->Internals->actionAbout_LidarView->setText(text);
  ss.str("");
  ss.clear();
}

//-----------------------------------------------------------------------------
void vvMainWindow::dragEnterEvent(QDragEnterEvent* evt)
{
  pqApplicationCore::instance()->getMainWindowEventManager()->dragEnterEvent(evt);
}

//-----------------------------------------------------------------------------
void vvMainWindow::showEvent(QShowEvent* evt)
{
  // TODO PV uses it to show Welcome Message once
  // could also be used for the "first startup" stuff
  static_cast<void>(evt);
}

//-----------------------------------------------------------------------------
void vvMainWindow::closeEvent(QCloseEvent* evt)
{
  pqApplicationCore::instance()->getMainWindowEventManager()->closeEvent(evt);
}

//-----------------------------------------------------------------------------
void vvMainWindow::dropEvent(QDropEvent* evt)
{
  QList<QUrl> urls = evt->mimeData()->urls();
  if (urls.isEmpty())
  {
    return;
  }

  QList<QString> files;

  foreach (QUrl url, urls)
  {
    if (!url.toLocalFile().isEmpty())
    {
      files.append(url.toLocalFile());
    }
  }

  // If we have no file we return
  if (files.empty() || files.first().isEmpty())
  {
    return;
  }

  if (files[0].endsWith(".pcap"))
  {
    lqOpenPcapReaction::createSourceFromFile(files[0]);
  }
  else if (files[0].endsWith(".pcd"))
  {
    QMessageBox::warning(nullptr, tr(""), tr("Unsupported input format"));
    return;
  }
  else
  {
    pqLoadDataReaction::loadData(files);
  }
}

//-----------------------------------------------------------------------------
void vvMainWindow::showHelpForProxy(const QString& groupname, const QString& proxyname)
{
  pqHelpReaction::showProxyHelp(groupname, proxyname);
}

//-----------------------------------------------------------------------------
void vvMainWindow::handleMessage(const QString&, int type)
{
  QDockWidget* dock = this->Internals->outputWidgetDock;
  if (!dock)
  {
    std::cout << "Error: NO DOCK for handleMessage()" << std::endl;
    return;
  }

  if (!dock->isVisible() && (type == QtCriticalMsg || type == QtFatalMsg || type == QtWarningMsg))
  {
    // if dock is not visible, we always pop it up as a floating dialog. This
    // avoids causing re-renders which may cause more errors and more confusion.
    QRect rectApp = this->geometry();

    QRect rectDock(
      QPoint(0, 0), QSize(static_cast<int>(rectApp.width() * 0.4), dock->sizeHint().height()));
    rectDock.moveCenter(
      QPoint(rectApp.center().x(), rectApp.bottom() - dock->sizeHint().height() / 2));
    dock->setFloating(true);
    dock->setGeometry(rectDock);
    dock->show();
  }
  if (dock->isVisible())
  {
    dock->raise();
  }
}

//-----------------------------------------------------------------------------
void vvMainWindow::toggleMVDecoration()
{
  // pqTabbedMultiViewWidget::setDecorationsVisibility()
  pqTabbedMultiViewWidget* mv = qobject_cast<pqTabbedMultiViewWidget*>(this->centralWidget());
  mv->setDecorationsVisibility(!mv->decorationsVisibility());
}
