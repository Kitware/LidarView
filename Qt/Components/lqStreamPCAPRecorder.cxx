/*=========================================================================

  Program: LidarView
  Module:  lqStreamPCAPRecorder.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqStreamPCAPRecorder.h"

#include <pqActiveObjects.h>
#include <pqPipelineSource.h>

#include <vtkSMLidarStreamProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMSessionProxyManager.h>

//-----------------------------------------------------------------------------
lqStreamPCAPRecorder::lqStreamPCAPRecorder(QObject* parent)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
lqStreamPCAPRecorder::~lqStreamPCAPRecorder() = default;

//-----------------------------------------------------------------------------
bool lqStreamPCAPRecorder::canRecord(pqPipelineSource* src)
{
  auto* proxy = vtkSMLidarStreamProxy::SafeDownCast(src->getProxy());
  if (!proxy)
  {
    return false;
  }

  if (vtkSMPropertyHelper(proxy, "PacketHandler", true).GetAsProxy())
  {
    return true;
  }

  // Recorder could be set at lidar proxy level.
  if (proxy->GetProperty("Recorder"))
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool lqStreamPCAPRecorder::startRecording(QString recordingPath,
  QList<pqPipelineSource*> streamList)
{
  this->RecordingFilePath = recordingPath;

  // Create a recorder proxy
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return false;
  }
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  this->RecorderProxy.TakeReference(pxm->NewProxy("misc", "PacketRecorder"));

  // Set a common packet writer proxy for all streams
  for (pqPipelineSource* src : streamList)
  {
    auto* proxy = vtkSMLidarStreamProxy::SafeDownCast(src->getProxy());
    vtkSMProxy* packetHandlerProxy = vtkSMPropertyHelper(proxy, "PacketHandler", true).GetAsProxy();
    if (packetHandlerProxy)
    {
      vtkSMPropertyHelper(packetHandlerProxy, "Recorder").Set(this->RecorderProxy);
      packetHandlerProxy->UpdateVTKObjects();
    }
    // Recorder could be set at lidar proxy level.
    else if (proxy->GetProperty("Recorder"))
    {
      vtkSMPropertyHelper(proxy, "Recorder").Set(this->RecorderProxy);
      proxy->UpdateVTKObjects();
    }
    else
    {
      qWarning() << "No recorder setter property found for " << proxy->GetVTKClassName();
      continue;
    }
  }

  this->BaseRecordingFileName = "Record";
  if (streamList.size() == 1)
  {
    auto* proxy = vtkSMLidarStreamProxy::SafeDownCast(streamList.front()->getProxy());
    this->BaseRecordingFileName = QString::fromStdString(proxy->GetLidarInformation());
  }

  // Set up writer proxy
  this->startWriterProxy();
  return true;
}

//-----------------------------------------------------------------------------
void lqStreamPCAPRecorder::startWriterProxy()
{
  if (!this->RecorderProxy)
  {
    return;
  }

  QString dateSuffix(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss"));
  this->LastRecordingFileName = this->BaseRecordingFileName + "-" + dateSuffix + ".pcap";

  QString recordingCompletePath(this->RecordingFilePath + "/" + this->LastRecordingFileName);
  const char* filename = recordingCompletePath.toUtf8().constData();
  vtkSMPropertyHelper(this->RecorderProxy, "RecordingFileName").Set(filename);
  this->RecorderProxy->UpdateProperty("RecordingFileName", true);
  this->RecorderProxy->InvokeCommand("StartRecording");
}

//-----------------------------------------------------------------------------
void lqStreamPCAPRecorder::stopWriterProxy()
{
  if (this->RecorderProxy)
  {
    this->RecorderProxy->InvokeCommand("StopRecording");
  }
}

//-----------------------------------------------------------------------------
void lqStreamPCAPRecorder::splitRecording()
{
  this->stopWriterProxy();
  // This will create a new filename with updated timestamp
  this->startWriterProxy();
}

//-----------------------------------------------------------------------------
void lqStreamPCAPRecorder::stopRecording()
{
  if (!this->RecorderProxy)
  {
    return;
  }

  // Tell vtkStreams to Stop Recording
  this->stopWriterProxy();

  // Delete recorder proxy
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server)
  {
    vtkSMSessionProxyManager* pxm = server->proxyManager();
    pxm->UnRegisterProxy("misc", "PacketRecorder", this->RecorderProxy);
  }
}

//-----------------------------------------------------------------------------
QString lqStreamPCAPRecorder::recordingFilename()
{
  return this->RecordingFilePath + "/" + this->LastRecordingFileName;
}
