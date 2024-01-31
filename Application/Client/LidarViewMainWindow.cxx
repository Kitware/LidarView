/*=========================================================================

  Program: LidarView
  Module:  LidarViewMainWindow.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "LidarViewMainWindow.h"
#include "ui_LidarViewMainWindow.h"

#include <vtkCommand.h>
#include <vtkPVGeneralSettings.h>
#include <vtkRemotingCoreConfiguration.h>

#include <pqApplicationCore.h>
#include <pqApplyBehavior.h>
#include <pqAutoLoadPluginXMLBehavior.h>
#include <pqAxesToolbar.h>
#include <pqCoreUtilities.h>
#include <pqLoadDataReaction.h>
#include <pqMainControlsToolbar.h>
#include <pqMainWindowEventManager.h>
#include <pqParaViewBehaviors.h>
#include <pqParaViewMenuBuilders.h>
#include <pqRepresentationToolbar.h>
#include <pqSetName.h>
#include <pqSettings.h>
#include <pqStandardViewFrameActionsImplementation.h>
#include <pqTimer.h>

// Python includes
#include "pqPythonDebugLeaksView.h"
#include "pqPythonShell.h"
#include <pvpythonmodules.h>
typedef pqPythonDebugLeaksView DebugLeaksViewType;

#include <QAction>
#include <QDebug>
#include <QDropEvent>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QToolBar>

#include "lqCommandLineOptionsBehavior.h"
#include "lqLidarViewManager.h"
#include "lqLidarViewMenuBuilders.h"
#include "lqLiveSourceScalarColoringBehavior.h"
#include "lqOpenLidarReaction.h"
#include "lqWelcomeDialog.h"

//-----------------------------------------------------------------------------
class LidarViewMainWindow::lqInternals : public Ui::LidarViewMainWindow
{
public:
  bool FirstShow;
  int CurrentGUIFontSize;
  QFont DefaultApplicationFont; // will be initialized to default app font in constructor.
  pqTimer UpdateFontSizeTimer;
  lqInternals()
    : FirstShow(true)
    , CurrentGUIFontSize(0)
  {
    this->UpdateFontSizeTimer.setInterval(0);
    this->UpdateFontSizeTimer.setSingleShot(true);
  }
};

//-----------------------------------------------------------------------------
LidarViewMainWindow::LidarViewMainWindow()
{
  // The debug leaks view should be constructed as early as possible
  // so that it can monitor vtk objects created at application startup.
  DebugLeaksViewType* leaksView = nullptr;
  if (vtksys::SystemTools::GetEnv("PV_DEBUG_LEAKS_VIEW"))
  {
    leaksView = new DebugLeaksViewType(this);
    leaksView->setWindowFlags(Qt::Window);
    leaksView->show();
  }

  // #if PARAVIEW_USE_PYTHON
  pvpythonmodules_load();
  // #endif

  // Init LidarView manager based on LVCore common manager
  new lqLidarViewManager(this);
  lqLidarViewManager::setLidarViewDefaultSettings();

  this->Internals = new lqInternals();
  this->Internals->setupUi(this);

  // Tabify some widgets (by default second widget will open where first widget is as a tab)
  this->tabifyDockWidget(this->Internals->pythonShellDock, this->Internals->memoryInspectorDock);
  this->tabifyDockWidget(this->Internals->pythonShellDock, this->Internals->findDataDock);
  this->tabifyDockWidget(this->Internals->pythonShellDock, this->Internals->informationDock);

  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->viewPropertiesDock);
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->displayPropertiesDock);

  // Change default properties panel modes (one dock for each)
  this->Internals->propertiesPanel->setPanelMode(pqPropertiesPanel::SOURCE_PROPERTIES);
  this->Internals->viewPropertiesPanel->setPanelMode(pqPropertiesPanel::VIEW_PROPERTIES);
  this->Internals->displayPropertiesPanel->setPanelMode(pqPropertiesPanel::DISPLAY_PROPERTIES);

  // Provide access to the find data, color-editor and python dock panels for the application.
  pqApplicationCore::instance()->registerManager("FIND_DATA_PANEL", this->Internals->findDataDock);
  pqApplicationCore::instance()->registerManager(
    "COLOR_EDITOR_PANEL", this->Internals->colorMapEditorDock);
  pqApplicationCore::instance()->registerManager(
    "PYTHON_SHELL_PANEL", this->Internals->pythonShellDock);

  QStringList preamble = { "from paraview.simple import *", "from lidarview.simple import *" };
  // Create pythonshell
  pqPythonShell* shell = new pqPythonShell(this);
  shell->setObjectName("pythonShell");
  shell->setFontSize(8);
  shell->setPreamble(preamble);
  this->Internals->pythonShellDock->setWidget(shell);
  // lqLidarViewManager::instance()->setPythonShell(shell);
  if (leaksView)
  {
    leaksView->setShell(shell);
  }

  // update UI when font size changes.
  vtkPVGeneralSettings* gsSettings = vtkPVGeneralSettings::GetInstance();
  pqCoreUtilities::connect(
    gsSettings, vtkCommand::ModifiedEvent, &this->Internals->UpdateFontSizeTimer, SLOT(start()));
  this->connect(&this->Internals->UpdateFontSizeTimer, SIGNAL(timeout()), SLOT(updateFontSize()));

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Populate application menus with actions.
  lqLidarViewMenuBuilders::buildFileMenu(*this->Internals->menuFileBase);
  lqLidarViewMenuBuilders::buildEditMenu(
    *this->Internals->menuEditBase, this->Internals->propertiesPanel);
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);
  pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menuMacros);
  lqLidarViewMenuBuilders::buildHelpMenu(*this->Internals->menuHelpBase);

  // Setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser->contextMenu());

  // Populate toolbars
  lqLidarViewMenuBuilders::buildToolbars(*this);

  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menuView, *this);

  // show output widget if we received an error message.
  this->connect(this->Internals->outputWidget,
    SIGNAL(messageDisplayed(const QString&, int)),
    SLOT(handleMessage(const QString&, int)));

  // Customize application behaviors - the other ones are kept to default
  pqParaViewBehaviors::setEnableCollaborationBehavior(false);
  pqParaViewBehaviors::setEnableViewStreamingBehavior(false);
  pqParaViewBehaviors::setEnableUsageLoggingBehavior(true);
  // This must be delayed in lqCommandLineOptionsBehavior so tests can run with LidarGridView
  pqParaViewBehaviors::setEnableCommandLineOptionsBehavior(false);
  new pqParaViewBehaviors(this, this);

  new lqCommandLineOptionsBehavior(this);
  new lqLiveSourceScalarColoringBehavior(this);

  // Restore last LidarView interface mode saved in settings
  lqLidarViewManager::instance()->restoreSavedInterfaceLayout();
}

//-----------------------------------------------------------------------------
LidarViewMainWindow::~LidarViewMainWindow()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void LidarViewMainWindow::dragEnterEvent(QDragEnterEvent* evt)
{
  pqApplicationCore::instance()->getMainWindowEventManager()->dragEnterEvent(evt);
}

//-----------------------------------------------------------------------------
void LidarViewMainWindow::dropEvent(QDropEvent* evt)
{
  QList<QUrl> urls = evt->mimeData()->urls();
  if (urls.isEmpty())
  {
    return;
  }

  QList<QString> files;
  Q_FOREACH (QUrl url, urls)
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
    lqOpenLidarReaction::openLidarPcap(files[0]);
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
void LidarViewMainWindow::showEvent(QShowEvent* evt)
{
  this->Superclass::showEvent(evt);

  // Force show app on front
  this->raise();
  this->activateWindow();

  if (this->Internals->FirstShow)
  {
    this->Internals->FirstShow = false;
    if (!vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry())
    {
      auto core = pqApplicationCore::instance();
      if (core->settings()->value("GeneralSettings.ShowWelcomeDialog", true).toBool())
      {
        pqTimer::singleShot(600, this, SLOT(showWelcomeDialog()));
      }

      this->updateFontSize();
    }
  }

  pqApplicationCore::instance()->getMainWindowEventManager()->showEvent(evt);
}

//-----------------------------------------------------------------------------
void LidarViewMainWindow::closeEvent(QCloseEvent* evt)
{
  pqApplicationCore::instance()->getMainWindowEventManager()->closeEvent(evt);
}

//-----------------------------------------------------------------------------
void LidarViewMainWindow::showWelcomeDialog()
{
  lqWelcomeDialog dialog(this);
  dialog.exec();
}

//-----------------------------------------------------------------------------
void LidarViewMainWindow::updateFontSize()
{
  auto& internals = *this->Internals;
  vtkPVGeneralSettings* gsSettings = vtkPVGeneralSettings::GetInstance();
  int fontSize = internals.DefaultApplicationFont.pointSize();
  if (gsSettings->GetGUIOverrideFont() && gsSettings->GetGUIFontSize() > 0)
  {
    fontSize = gsSettings->GetGUIFontSize();
  }
  if (fontSize <= 0)
  {
    qDebug() << "Ignoring invalid font size: " << fontSize;
    return;
  }

  if (internals.CurrentGUIFontSize != fontSize)
  {
    QFont newFont = this->font();
    newFont.setPointSize(fontSize);
    this->setFont(newFont);
    this->Internals->CurrentGUIFontSize = fontSize;
  }

  // Consoles font size
  pqPythonShell* shell = qobject_cast<pqPythonShell*>(this->Internals->pythonShellDock->widget());
  shell->setFontSize(fontSize);

  pqOutputWidget* outputWidget =
    qobject_cast<pqOutputWidget*>(this->Internals->outputWidgetDock->widget());
  outputWidget->setFontSize(fontSize);
}

//-----------------------------------------------------------------------------
void LidarViewMainWindow::handleMessage(const QString&, int type)
{
  QDockWidget* dock = this->Internals->outputWidgetDock;
  pqOutputWidget* outputWidget = qobject_cast<pqOutputWidget*>(dock->widget());
  if (dock->isFloating() && !outputWidget->shouldOpenForNewMessages())
  {
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
