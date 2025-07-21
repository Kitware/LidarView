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

#include "lqStreamRecordDialog.h"

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
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

namespace
{
//-----------------------------------------------------------------------------
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
  this->SplitRecordTimer = new QTimer(this);
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

  if (streamList.isEmpty())
  {
    return false;
  }
  this->BaseRecordingFileName = "Record";
  if (streamList.size() == 1)
  {
    this->BaseRecordingFileName = QString::fromStdString(streamList.front()->GetLidarInformation());
  }

  lqStreamRecordDialog dialog(pqCoreUtilities::mainWidget());
  if (dialog.exec() != pqFileDialog::Accepted)
  {
    return false;
  }

  this->RecordingFilePath = dialog.getFilepath();
  QFileInfo fileInfo(this->RecordingFilePath);
  if (!fileInfo.exists() || !fileInfo.isDir())
  {
    qCritical() << "Recording Filename is not valid, aborting.";
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
  this->startWriterProxy();
  this->IsRecording = true;

  if (dialog.isSplitRecordingEnabled())
  {
    unsigned int interval = dialog.getSplitInterval();
    this->connect(
      this->SplitRecordTimer, &QTimer::timeout, this, &lqStreamRecordController::onSplitRecording);
    this->SplitRecordTimer->start(interval * 60 * 1000);
  }
  return true;
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::startWriterProxy()
{
  QString dateSuffix(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss"));
  this->LastRecordingFileName = this->BaseRecordingFileName + "-" + dateSuffix + ".pcap";

  QString recordingCompletePath(this->RecordingFilePath + "/" + this->LastRecordingFileName);
  const char* filename = recordingCompletePath.toUtf8().constData();
  vtkSMPropertyHelper(this->RecorderProxy, "RecordingFileName").Set(filename);
  this->RecorderProxy->UpdateProperty("RecordingFileName", true);
  this->RecorderProxy->InvokeCommand("StartRecording");
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::stopWriterProxy()
{
  if (this->RecorderProxy)
  {
    this->RecorderProxy->InvokeCommand("StopRecording");
    Q_EMIT this->recordingFileFinished(this->RecordingFilePath + "/" + this->LastRecordingFileName);
  }
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::onSplitRecording()
{
  this->stopWriterProxy();
  // This will create a new filename with updated timestamp
  this->startWriterProxy();
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::stopRecording()
{
  if (!this->RecorderProxy)
  {
    return;
  }

  // Tell vtkStreams to Stop Recording
  this->stopWriterProxy();
  this->IsRecording = false;

  // Stop timer
  this->SplitRecordTimer->disconnect();
  this->SplitRecordTimer->stop();

  // Delete recorder proxy
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server)
  {
    vtkSMSessionProxyManager* pxm = server->proxyManager();
    pxm->UnRegisterProxy("internal_interpreter", "PacketRecorder", this->RecorderProxy);
  }

  // Display a feedback message to the user when the recording is stopped
  QMessageBox stopRecordMsg;
  const QString txt = "Stream data have been saved in the directory:\n\n";
  stopRecordMsg.setWindowTitle("Stop stream recording");
  stopRecordMsg.setText(txt + this->RecordingFilePath);
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
