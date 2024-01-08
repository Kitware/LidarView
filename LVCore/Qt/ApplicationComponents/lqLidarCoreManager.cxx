/*=========================================================================

   Program: LidarView
   Module:  lqLidarCoreManager.cxx

   Copyright (c) Kitware Inc.
   All rights reserved.

   LidarView is a free software; you can redistribute it and/or modify it
   under the terms of the LidarView license.

   See LICENSE for the full LidarView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "lqLidarCoreManager.h"

// TODO This pqPythonShell Include is higher to prevent an unindentified macro conflict
#include <pqPythonShell.h>

#include <lqHelper.h>
#include <lqMeasurementGridReaction.h>
#include <lqSensorListWidget.h>
#include <vtkGridSource.h>
#include <vtkLVPython.h>
#include <vtkLVVersion.h>
#include <vtkLidarReader.h>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPVApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqSettings.h>
#include <pqView.h>

#include <vtkFieldData.h>
#include <vtkPVVersion.h> //  needed for PARAVIEW_VERSION
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>

#include <vtkPointData.h>
#include <vtkTimerLog.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QTimer>

#include <sstream>

//-----------------------------------------------------------------------------
class lqLidarCoreManager::pqInternal
{
};

//-----------------------------------------------------------------------------
QPointer<lqLidarCoreManager> lqLidarCoreManager::Instance = nullptr;

//-----------------------------------------------------------------------------
lqLidarCoreManager* lqLidarCoreManager::instance()
{
  return lqLidarCoreManager::Instance;
}

//-----------------------------------------------------------------------------
lqLidarCoreManager::lqLidarCoreManager(QObject* parent)
  : QObject(parent)
{
  // Only 1 lqLidarCoreManager instance can be created.
  Q_ASSERT(lqLidarCoreManager::Instance == nullptr);
  lqLidarCoreManager::Instance = this;

  this->Internal = new pqInternal;

  // Create lqSensorListWidget, a critical component of LidarView core.
  // App can choose wether or not to show it or not, and how to present it (e.g QWidgetDock)
  new lqSensorListWidget();

  // PrependPythonPath will complete automatically path to right python lib path depending os.
  vtkLVPython::PrependLVModulesPythonPath();
}

//-----------------------------------------------------------------------------
lqLidarCoreManager::~lqLidarCoreManager()
{
  if (lqLidarCoreManager::Instance == this)
  {
    lqLidarCoreManager::Instance = nullptr;
  } // WIP Paraview has similar code but 'else' case should not happend at all

  delete this->Internal;
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::setPythonShell(pqPythonShell* shell)
{
  this->pythonShell = shell;
}

//-----------------------------------------------------------------------------
pqPythonShell* lqLidarCoreManager::getPythonShell()
{
  return this->pythonShell;
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::runPython(const QString& statements)
{
  if (this->pythonShell)
  {
    this->pythonShell->executeScript(statements);
  }
  else
  {
    qCritical() << "LidarView python shell has not been set, cannot run:\n" << statements;
  }
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::forceShowShell()
{
  if (this->pythonShell)
  {
    this->pythonShell->show();
    this->pythonShell->raise();
    this->pythonShell->activateWindow();
  }
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onMeasurementGrid(bool gridVisible)
{
  lqMeasurementGridReaction::updateGridVisibility(gridVisible);
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onResetCameraLidar()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!view)
  {
    return;
  }

  // See pqRenderView::resetViewDirection
  // Position at 30 degrees [0, -(squareRoot(3)/2)*dist, (1/2)*dist]
  vtkSMProxy* proxy = view->getProxy();
  constexpr double dist = 100;
  double pos[3] = { 0, -0.866025 * dist, (1.0 / 2.0) * dist };
  double focal_point[3] = { 0, 0, 0 };
  double view_up[3] = { 0, 0, 1 };
  vtkSMPropertyHelper(proxy, "CameraPosition").Set(pos, 3);
  vtkSMPropertyHelper(proxy, "CameraFocalPoint").Set(focal_point, 3);
  vtkSMPropertyHelper(proxy, "CameraViewUp").Set(view_up, 3);
  proxy->UpdateVTKObjects();
  view->render();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onResetCameraToForwardView()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!view)
  {
    return;
  }

  vtkSMProxy* proxy = view->getProxy();
  double pos[3] = { 0, -72, 18.0 };
  double focal_point[3] = { 0, 0, 0 };
  double view_up[3] = { 0, 0.27, 0.96 };
  vtkSMPropertyHelper(proxy, "CameraPosition").Set(pos, 3);
  vtkSMPropertyHelper(proxy, "CameraFocalPoint").Set(focal_point, 3);
  vtkSMPropertyHelper(proxy, "CameraViewUp").Set(view_up, 3);
  proxy->UpdateVTKObjects();
  view->setCenterOfRotation(0, 0, 0);

  view->render();
}

//-----------------------------------------------------------------------------
pqServer* lqLidarCoreManager::getActiveServer()
{
  pqApplicationCore* app = pqApplicationCore::instance();
  pqServerManagerModel* smModel = app->getServerManagerModel();
  pqServer* server = smModel->getItemAtIndex<pqServer*>(0);
  return server;
}

//-----------------------------------------------------------------------------
QWidget* lqLidarCoreManager::getMainWindow()
{
  Q_FOREACH (QWidget* topWidget, QApplication::topLevelWidgets())
  {
    if (qobject_cast<QMainWindow*>(topWidget))
    {
      return topWidget;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onEnableCrashAnalysis(bool crashAnalysisEnabled)
{
  pqSettings* const Settings = pqApplicationCore::instance()->settings();
  Settings->setValue("LidarPlugin/MainWindow/EnableCrashAnalysis", crashAnalysisEnabled);
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onCloseAllData()
{
  // Remove lidars
  RemoveAllProxyTypeFromPipelineBrowser<vtkLidarReader>();
  RemoveAllProxyTypeFromPipelineBrowser<vtkLidarStream>();
  RemoveAllProxyTypeFromPipelineBrowser<vtkLidarPoseReader>();

  // Reset Camera
  this->onResetCameraToForwardView();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onResetDefaultSettings()
{
  QMessageBox messageBox;
  messageBox.setIcon(QMessageBox::Warning);
  std::stringstream ss;
  ss << "This action will reset all settings. "
     << "This action requires " << SOFTWARE_NAME << " to restart to be completly reset. "
     << "Every unsaved changes will be lost. Are you sure you want to reset " << SOFTWARE_NAME
     << " settings?";
  messageBox.setText(ss.str().c_str());
  messageBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

  if (messageBox.exec() == QMessageBox::Ok)
  {
    // Clear Settings, sets the no restore flag "pqApplicationCore.DisableSettings"
    pqApplicationCore::instance()->clearSettings();

    // Quit the current application instance and restart a new one.
    qApp->quit();

    QStringList appArguments = qApp->arguments();
    QString appName = appArguments.at(0);
    appArguments.pop_front();
    QProcess::startDetached(appName, appArguments);
  }
}
