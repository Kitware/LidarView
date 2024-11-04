/*=========================================================================

  Program: LidarView
  Module:  lqStreamRecordController.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqStreamRecordController.h"

#include "vtkSMLidarStreamProxy.h"
#include "vtkStream.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqPipelineSource.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>

#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMSessionProxyManager.h>

#include <QDateTime>
#include <QDebug>
#include <QMessageBox>

namespace
{
bool isLastStream(pqPipelineSource* src)
{
  if (!vtkSMLidarStreamProxy::SafeDownCast(src->getProxy()))
  {
    return false;
  }

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (vtkSMLidarStreamProxy::SafeDownCast(src->getProxy()))
    {
      return false;
    }
  }
  return true;
}
}

//-----------------------------------------------------------------------------
lqStreamRecordController::lqStreamRecordController(QObject* parent)
  : Superclass(parent)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel,
    &pqServerManagerModel::sourceRemoved,
    this,
    &lqStreamRecordController::onSourceRemoved);
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::onRecordStream(bool status)
{
  if (status)
  {
    bool isRecording = this->startRecording();
    Q_EMIT this->recording(isRecording);
  }
  else
  {
    this->stopRecording();
  }
}

//-----------------------------------------------------------------------------
bool lqStreamRecordController::startRecording()
{
  QList<vtkSMLidarStreamProxy*> streamList;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    auto proxy = vtkSMLidarStreamProxy::SafeDownCast(src->getProxy());
    if (proxy != nullptr)
    {
      streamList.append(proxy);
    }
  }

  QString defaultFileName = "";
  if (streamList.isEmpty())
  {
    return false;
  }
  if (streamList.size() == 1)
  {
    defaultFileName = QString::fromStdString(streamList.at(0)->GetLidarInformation());
  }
  defaultFileName = defaultFileName.isEmpty() ? "Record-" : defaultFileName + "-";
  defaultFileName += QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss");
  defaultFileName += ".pcap";

  QString title = "Choose where to record: " + defaultFileName;
  auto dialog = pqFileDialog(pqActiveObjects::instance().activeServer(),
    pqCoreUtilities::mainWidget(),
    title,
    QDir::currentPath());
  dialog.setFileMode(pqFileDialog::Directory);
  if (dialog.exec() != pqFileDialog::Accepted)
  {
    return false;
  }
  this->RecordingFilename = dialog.getSelectedFiles()[0] + "/" + defaultFileName;
  if (this->RecordingFilename.isEmpty())
  {
    qCritical() << "Recording Filename is empty, aborting.";
    return false;
  }

  // Create a recorder proxy
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return false;
  }
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  this->RecorderProxy.TakeReference(pxm->NewProxy("internal_interpreter", "PacketRecorder"));

  // Set a common packet writer proxy for all streams
  Q_FOREACH (vtkSMLidarStreamProxy* proxy, streamList)
  {
    vtkSMProxy* packetHandlerProxy = vtkSMPropertyHelper(proxy, "PacketHandler").GetAsProxy();
    if (!packetHandlerProxy)
    {
      qWarning() << "No packet handler proxy found for " << proxy->GetVTKClassName();
      continue;
    }
    vtkSMPropertyHelper(packetHandlerProxy, "Recorder").Set(this->RecorderProxy);
    packetHandlerProxy->UpdateVTKObjects();
  }

  // Set up writer proxy
  const char* filename = this->RecordingFilename.toUtf8().constData();
  vtkSMPropertyHelper(this->RecorderProxy, "RecordingFileName").Set(filename);
  this->RecorderProxy->UpdateProperty("RecordingFileName", true);
  this->RecorderProxy->InvokeCommand("StartRecording");

  this->IsRecording = true;
  return true;
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::stopRecording()
{
  if (!this->RecorderProxy)
  {
    return;
  }

  // Tell vtkStreams to Stop Recording
  this->RecorderProxy->InvokeCommand("StopRecording");
  this->IsRecording = false;

  // Delete recorder proxy
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server)
  {
    vtkSMSessionProxyManager* pxm = server->proxyManager();
    pxm->UnRegisterProxy("internal_interpreter", "PacketRecorder", this->RecorderProxy);
  }

  // Display a feedback message to the user when the recording is stopped
  QMessageBox stopRecordMsg;
  const QString txt = "Stream data have been saved in the file:\n\n";
  stopRecordMsg.setWindowTitle("Stop stream recording");
  stopRecordMsg.setText(txt + this->RecordingFilename);
  stopRecordMsg.setStandardButtons(QMessageBox::Ok);
  stopRecordMsg.exec();

  Q_EMIT this->recording(false);
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::onSourceRemoved(pqPipelineSource* src)
{
  if (::isLastStream(src) && this->IsRecording)
  {
    this->stopRecording();
  }
}
