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

#include "LASFileWriter.h"
#include <vtkLidarReader.h>
#include <lqPythonQtDecorators.h>
#include <lqSensorListWidget.h>
#include <lqHelper.h>
#include <vtkGridSource.h>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPVApplicationCore.h>
#include <pqParaViewBehaviors.h>
#include <pqPersistentMainWindowStateBehavior.h>
#include <pqPipelineSource.h>
#include <pqPythonManager.h>
#include <pqRenderView.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqSettings.h>
#include <pqView.h>

#include <vtkFieldData.h>
#include <vtkPVConfig.h> //  needed for PARAVIEW_VERSION
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>

#include <vtkPointData.h>
#include <vtkPythonInterpreter.h>
#include <vtkTimerLog.h>


#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QTimer>

#include <sstream>

// Use LV_PYTHON_VERSION supplied at build time
#ifndef LV_PYTHON_VERSION
  #error "LV_PYTHON_VERSION not defined"
#endif
static_assert( LV_PYTHON_VERSION, "LV_PYTHON_VERSION is not defined" ); // For good measure

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
void lqLidarCoreManager::schedulePythonStartup()
{
  QTimer::singleShot(0, this, SLOT(pythonStartup()));
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
  if(this->pythonShell)
  {
    this->pythonShell->executeScript(statements);
  }else{
    qCritical() <<"LidarView python shell has not been set, cannot run:\n" << statements ;
  }
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::forceShowShell(){
  if(this->pythonShell)
  {
    this->pythonShell->show();
    this->pythonShell->raise();
    this->pythonShell->activateWindow();
  }
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::createMainRenderView()
{
  vtkSMSessionProxyManager* pxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSMProxy* renderviewsettings = pxm->GetProxy("RenderViewSettings");
  assert(renderviewsettings);
  vtkSMPropertyHelper(renderviewsettings, "ResolveCoincidentTopology").Set(0); //WIP Is This necessary

  pqRenderView* view = qobject_cast<pqRenderView*>(
    pqApplicationCore::instance()->getObjectBuilder()->createView(pqRenderView::renderViewType(), pqActiveObjects::instance().activeServer())
  );
  assert(view);
  pqActiveObjects::instance().setActiveView(view);
  pqApplicationCore::instance()->getObjectBuilder()->addToLayout(view);

  double bgcolor[3] = { 0, 0, 0 };
  vtkSMPropertyHelper(view->getProxy(), "Background").Set(bgcolor, 3);
  vtkSMPropertyHelper(view->getProxy(), "CenterAxesVisibility").Set(0);
  //vtkSMPropertyHelper(view->getProxy(),"MultiSamples").Set(4); //WIP set to 0 1, 4 ?
  view->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::pythonStartup()
{
  // Python module Paths
  QStringList pythonDirs;
  pythonDirs << QCoreApplication::applicationDirPath()
             << QCoreApplication::applicationDirPath() + "/../Libraries" // use lidarpluginpython module from packaging MacOS
             << QCoreApplication::applicationDirPath() + "/../Python/" // use lidarview module from install MacOS
             << QCoreApplication::applicationDirPath() + "/../lib/pythonLV_PYTHON_VERSION/site-packages/" // use lidarview module from install Linux
             << QCoreApplication::applicationDirPath() + "/Lib/site-packages/"; // use lidarview module from install Windows
  foreach (const QString& dirname, pythonDirs)
  {
    if (QDir(dirname).exists())
    {
      vtkPythonInterpreter::PrependPythonPath(dirname.toUtf8().data());
    }
  }

  // Python Decorators
  PythonQt::self()->addDecorators(new lqPythonQtDecorators(this));

  // Start applogic
  this->runPython(QString(
      "import PythonQt\n"
      "QtGui = PythonQt.QtGui\n"
      "QtCore = PythonQt.QtCore\n"
      "import lidarview.applogic as lv\n"
      "lv.start()\n"));

}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onMeasurementGrid(bool gridVisible)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("LidarPlugin/MeasurementGrid/Visibility", gridVisible);

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (IsProxy<vtkGridSource *>(src->getProxy()))
    {
      controller->SetVisibility(
        vtkSMSourceProxy::SafeDownCast(src->getProxy()), 0,
        pqActiveObjects::instance().activeView()->getViewProxy(),
        gridVisible
      );
    }
  }
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onResetCameraLidar()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if(!view){return;}

  // See pqRenderView::resetViewDirection
  // Position at 30 degrees [0, -(squareRoot(3)/2)*dist, (1/2)*dist]
  vtkSMProxy* proxy = view->getProxy();
  constexpr double dist = 100;
  double pos[3]         = { 0, -0.866025 * dist, (1.0/2.0) * dist };
  double focal_point[3] = { 0,0,0 };
  double view_up[3]     = { 0,0,1 };
  vtkSMPropertyHelper(proxy, "CameraPosition").Set(pos, 3);
  vtkSMPropertyHelper(proxy, "CameraFocalPoint").Set(focal_point, 3);
  vtkSMPropertyHelper(proxy, "CameraViewUp").Set(view_up, 3);
  proxy->UpdateVTKObjects();
  view->resetCamera();
  view->render();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onResetCameraToForwardView()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if(!view){return;}

  vtkSMProxy* proxy = view->getProxy();
  double pos[3]         = { 0, -72, 18.0 };
  double focal_point[3] = { 0,0,0 };
  double view_up[3]     = { 0, 0.27, 0.96 };
  vtkSMPropertyHelper(proxy, "CameraPosition").Set(pos, 3);
  vtkSMPropertyHelper(proxy, "CameraFocalPoint").Set(focal_point, 3);
  vtkSMPropertyHelper(proxy, "CameraViewUp").Set(view_up, 3);
  proxy->UpdateVTKObjects();
  view->setCenterOfRotation(0, 0, 0);

  view->render();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::onResetCenterToLidarCenter()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if(!view){return;}

  // Set Axis Center to 0 0 0
  double center[3] = {0.0,0.0,0.0};
  view->setCenterOfRotation(center);
  view->render();

  // Set Camera Rotation Focal Point
  view->setCenterOfRotation(0, 0, 0);
  view->render();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::saveFramesToPCAP(
  vtkSMSourceProxy* proxy, int startFrame, int endFrame, const QString& filename)
{
  if (!proxy)
  {
    return;
  }

  vtkLidarReader* reader = vtkLidarReader::SafeDownCast(proxy->GetClientSideObject());
  if (!reader)
  {
    return;
  }

  reader->Open();
  reader->SaveFrame(startFrame, endFrame, filename.toUtf8().data());
  reader->Close();
}

//-----------------------------------------------------------------------------
void lqLidarCoreManager::saveFramesToLAS(vtkLidarReader* reader, vtkPolyData* position,
  int startFrame, int endFrame, const QString& filename, int positionMode)
{
  if (!reader || (positionMode > 0 && !position))
  {
    return;
  }

  // initialize origin point
  double northing, easting, height;
  easting = northing = height = 0;

  // projection transform parameters
  int gcs, in, out, utmZone;
  gcs = in = out = utmZone = 0;

  // data accuracy
  double neTol, hTol;
  hTol = neTol = 1e-3;

  bool isLatLon = false;

  LASFileWriter writer;
  writer.Open(qPrintable(filename));

  // not sensor relative; it can be
  // relative registered data or
  // georeferenced data
  if (positionMode > 0)
  {

    // Georeferenced data
    if (positionMode > 1)
    {
      // Since the data are georeferenced here, we must
      // check that a position reader is provided
      if (position)
      {
        vtkDataArray* const zoneData = position->GetFieldData()->GetArray("zone");
        vtkDataArray* const eastingData = position->GetPointData()->GetArray("easting");
        vtkDataArray* const northingData = position->GetPointData()->GetArray("northing");
        vtkDataArray* const heightData = position->GetPointData()->GetArray("height");

        if (zoneData && zoneData->GetNumberOfTuples() && eastingData &&
          eastingData->GetNumberOfTuples() && northingData && northingData->GetNumberOfTuples() &&
          heightData && heightData->GetNumberOfTuples())
        {
          // We assume that eastingData, norhtingData and heightData are in system reference
          // coordinates (srs) of UTM zoneData
          utmZone = static_cast<int>(zoneData->GetComponent(0, 0));

          // should in some cases use 32700? 32600 is for northern UTM zone, 32700 for southern UTM zone
          gcs = 32600 + utmZone;

          out = gcs;
          if (positionMode == 3) // Absolute lat/lon
          {
            in = gcs; // ...or 32700?
            out = 4326; // lat/lon (espg id code for lat-long-alt coordinates)
            neTol = 1e-8; // about 1mm;
            isLatLon = true;
          }

          northing = northingData->GetComponent(0, 0);
          easting = eastingData->GetComponent(0, 0);
          height = heightData->GetComponent(0, 0);
        }
      }
    }
  }

  std::cout << "origin : [" << northing << ";" << easting << ";" << height << "]" << std::endl;
  std::cout << "gcs : " << gcs << std::endl;

  writer.SetPrecision(neTol, hTol);
  writer.SetGeoConversionUTM(utmZone, isLatLon);
  writer.SetOrigin(easting, northing, height);

  QProgressDialog progress("Exporting LAS...", "Abort Export", startFrame,
    startFrame + (endFrame - startFrame) * 2, getMainWindow());
  progress.setWindowModality(Qt::WindowModal);

  reader->Open();
  for (int frame = startFrame; frame <= endFrame; ++frame)
  {
    progress.setValue(frame);

    if (progress.wasCanceled())
    {
      reader->Close();
      return;
    }

    const vtkSmartPointer<vtkPolyData>& data = reader->GetFrame(frame);
    writer.UpdateMetaData(data.GetPointer());
  }

  writer.FlushMetaData();

  for (int frame = startFrame; frame <= endFrame; ++frame)
  {
    progress.setValue(endFrame + (frame - startFrame));

    if (progress.wasCanceled())
    {
      reader->Close();
      return;
    }

    const vtkSmartPointer<vtkPolyData>& data = reader->GetFrame(frame);
    writer.WriteFrame(data.GetPointer());
  }

  reader->Close();
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
  foreach (QWidget* topWidget, QApplication::topLevelWidgets())
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
void lqLidarCoreManager::onResetDefaultSettings()
{
  QMessageBox messageBox;
  messageBox.setIcon(QMessageBox::Warning);
  std::stringstream ss;
  ss << "This action will reset all settings. "
     << "This action requires " << SOFTWARE_NAME << " to restart to be completly reset. "
     << "Every unsaved changes will be lost. Are you sure you want to reset " << SOFTWARE_NAME << " settings?";
  messageBox.setText(ss.str().c_str());
  messageBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

  if (messageBox.exec() == QMessageBox::Ok)
  {
    // Clear Settings, sets the no restore flag "pqApplicationCore.DisableSettings"
    pqApplicationCore::instance()->clearSettings();

    // Quit the current application instance and restart a new one.
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0]);
  }
}
