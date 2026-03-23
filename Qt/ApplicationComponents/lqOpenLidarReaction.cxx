/*=========================================================================

  Program: LidarView
  Module:  lqOpenLidarReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqOpenLidarReaction.h"

#include <QApplication>
#include <QDebug>
#include <QProgressDialog>
#include <QString>

#include <vtkCommand.h>
#include <vtkPVProgressHandler.h>
#include <vtkPVSession.h>
#include <vtkProcessModule.h>
#include <vtkSMAnimationSceneProxy.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMProxy.h>
#include <vtkSMReaderFactory.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMTrace.h>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqServer.h>
#include <pqUndoStack.h>
#include <pqView.h>

#include "lqHelper.h"
#include "lqLidarConfigurationDialog.h"
#include "lqLidarCoreManager.h"
#include "lqRecentlyUsedPcapLoader.h"

namespace
{
void InitAndDisplaySource(pqPipelineSource* source, vtkSMProxy* prototype, bool updateAnimation)
{
  vtkSMProxy* proxy = source->getSourceProxy();
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
  proxy->Copy(prototype);
  proxy->UpdateVTKObjects();

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  if (updateAnimation)
  {
    pqServer* server = pqActiveObjects::instance().activeServer();
    vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps(
      controller->GetAnimationScene(server->session()));
  }

  source->updatePipeline();
  source->setModifiedState(pqProxy::UNMODIFIED);
  pqView* view = pqActiveObjects::instance().activeView();
  controller->Show(source->getSourceProxy(), 0, view->getViewProxy());
  pqActiveObjects::instance().setActiveSource(source);
}

// Keep sending signals to the qt app when loading heavy file
class vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
  {
    vtkObserver* obs = new vtkObserver();
    return obs;
  }

  void Execute(vtkObject*, unsigned long eventId, void*) override
  {
    if (eventId == vtkCommand::ProgressEvent)
    {
      QApplication::instance()->processEvents();
    }
  }
};
}

//-----------------------------------------------------------------------------
lqOpenLidarReaction::lqOpenLidarReaction(QAction* parent, vtkSMInterpretersManagerProxy::Mode mode)
  : Superclass(parent)
  , InterpreterMode(mode)
{
}

//-----------------------------------------------------------------------------
bool lqOpenLidarReaction::openLidarPcap()
{
  // Select a LiDAR file
  pqFileDialog dialog(pqActiveObjects::instance().activeServer(),
    pqCoreUtilities::mainWidget(),
    tr("Open LiDAR File"),
    QString(),
    tr("Wireshark Capture (*.pcap *.pcapng)"),
    false);
  dialog.setObjectName("lqLidarFileOpenDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() == QDialog::Rejected)
  {
    return false;
  }
  QString filename = dialog.getSelectedFiles().at(0);

  return lqOpenLidarReaction::openLidarPcap(filename);
}

//-----------------------------------------------------------------------------
bool lqOpenLidarReaction::openLidarPcap(const QString& filename,
  pqServer* server,
  vtkSMProxy* defaultProxy)
{
  if (!server)
  {
    server = pqActiveObjects::instance().activeServer();
  }

  if (!vtkSMReaderFactory::TestFileReadability(filename.toUtf8().data(), server->session()))
  {
    qWarning() << "File '" << filename << "' cannot be read.";
    return false;
  }

  // Dialog to choose interpreter and configure LiDAR proxy
  lqLidarConfigurationDialog dialog(
    pqCoreUtilities::mainWidget(), vtkSMInterpretersManagerProxy::Mode::READER, defaultProxy);
  if (!dialog.exec())
  {
    return false;
  }

  QProgressDialog progress("Reading pcap", "", 0, 0, pqCoreUtilities::mainWidget());
  progress.setCancelButton(nullptr);
  progress.setModal(true);
  progress.show();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVSession* session = vtkPVSession::SafeDownCast(pm->GetSession());
  if (!session)
  {
    return false;
  }

  vtkSMProxy* prototype = dialog.getProxy();
  if (!prototype)
  {
    qCritical() << "The lidar proxy could not be created";
    return false;
  }

  vtkSmartPointer<vtkPVProgressHandler> handler = session->GetProgressHandler();
  handler->PrepareProgress();
  vtkNew<::vtkObserver> obs;
  unsigned long tag = handler->AddObserver(vtkCommand::ProgressEvent, obs);

  // We remove all lidarStream (and every filter depending on them) in the pipeline.
  RemoveAllProxyTypeFromPipelineBrowser<vtkLidarStream>();
  if (!dialog.isMultiSensorsEnabled())
  {
    RemoveAllProxyTypeFromPipelineBrowser<vtkLidarReader>();
  }

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqPipelineSource* source =
    builder->createReader(prototype->GetXMLGroup(), prototype->GetXMLName(), { filename }, server);
  if (source)
  {
    ::InitAndDisplaySource(source, prototype, true);
    lqRecentlyUsedPcapLoader::addPcapFileToRecentResources(server, filename, prototype);
  }

  handler->RemoveObserver(tag);
  handler->LocalCleanupPendingProgress();
  progress.close();
  return true;
}

//-----------------------------------------------------------------------------
bool lqOpenLidarReaction::openLidarStream()
{
  // Dialog to choose interpreter and configure LiDAR proxy
  lqLidarConfigurationDialog dialog(
    pqCoreUtilities::mainWidget(), vtkSMInterpretersManagerProxy::Mode::STREAM);
  if (!dialog.exec())
  {
    return false;
  }

  vtkSMProxy* prototype = dialog.getProxy();
  if (!prototype)
  {
    qCritical() << "The lidar proxy could not be created";
    return false;
  }
  // We remove all lidarReader (and every filter depending on them) in the pipeline.
  RemoveAllProxyTypeFromPipelineBrowser<vtkLidarReader>();
  if (!dialog.isMultiSensorsEnabled())
  {
    RemoveAllProxyTypeFromPipelineBrowser<vtkLidarStream>();
  }

  pqServer* server = pqActiveObjects::instance().activeServer();
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqPipelineSource* source =
    builder->createSource(prototype->GetXMLGroup(), prototype->GetXMLName(), server);
  if (!source)
  {
    return false;
  }
  ::InitAndDisplaySource(source, prototype, true);
  return true;
}

//-----------------------------------------------------------------------------
void lqOpenLidarReaction::onTriggered()
{
  switch (this->InterpreterMode)
  {
    case vtkSMInterpretersManagerProxy::Mode::READER:
      lqOpenLidarReaction::openLidarPcap();
      break;
    case vtkSMInterpretersManagerProxy::Mode::STREAM:
      lqOpenLidarReaction::openLidarStream();
      break;
    default:
      qCritical() << "Interpretation mode not implemented.";
      break;
  }
}
