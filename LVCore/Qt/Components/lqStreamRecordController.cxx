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

#include "lqStreamRecordDialog.h"
#include "lqStreamRecorderInterface.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqInterfaceTracker.h>
#include <pqPipelineSource.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>

#include <vtkSMProxy.h>

#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

namespace
{
//-----------------------------------------------------------------------------
QList<lqStreamRecorderInterface*> getUsableRecorders()
{
  pqInterfaceTracker* itk = pqApplicationCore::instance()->interfaceTracker();
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  QList<lqStreamRecorderInterface*> validInterface;
  for (lqStreamRecorderInterface* iface : itk->interfaces<lqStreamRecorderInterface*>())
  {
    bool hasSource = false;
    for (pqPipelineSource* src : smmodel->findItems<pqPipelineSource*>())
    {
      if (iface->canRecord(src))
      {
        hasSource = true;
        break;
      }
    }
    if (hasSource)
    {
      validInterface.append(iface);
    }
  }
  return validInterface;
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
  QList<lqStreamRecorderInterface*> ifaces = ::getUsableRecorders();
  if (ifaces.empty())
  {
    QMessageBox stopRecordMsg;
    stopRecordMsg.setWindowTitle("Recording failed");
    stopRecordMsg.setText("No valid recorder was found for this type of stream!");
    stopRecordMsg.setStandardButtons(QMessageBox::Ok);
    stopRecordMsg.exec();
    return false;
  }

  lqStreamRecordDialog dialog(pqCoreUtilities::mainWidget(), ifaces);
  if (dialog.exec() != pqFileDialog::Accepted)
  {
    return false;
  }

  this->CurrentRecorder = dialog.getSelectedInterface();
  this->ActiveRecordingSources = dialog.getSelectedSources();
  if (this->ActiveRecordingSources.isEmpty())
  {
    return false;
  }

  QString recordFilePath = dialog.getFilepath();
  QFileInfo fileInfo(recordFilePath);
  if (!fileInfo.exists() || !fileInfo.isDir())
  {
    qCritical() << "Recording Filename is not valid, aborting.";
    return false;
  }

  if (!this->CurrentRecorder->startRecording(recordFilePath, this->ActiveRecordingSources))
  {
    return false;
  }
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
void lqStreamRecordController::onSplitRecording()
{
  if (this->CurrentRecorder)
  {
    this->CurrentRecorder->splitRecording();
    Q_EMIT this->recordingFileFinished(this->CurrentRecorder->recordingFilename());
  }
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::stopRecording()
{
  if (!this->CurrentRecorder)
  {
    return;
  }

  this->CurrentRecorder->stopRecording();
  this->IsRecording = false;

  Q_EMIT this->recordingFileFinished(this->CurrentRecorder->recordingFilename());

  // Stop timer
  this->SplitRecordTimer->disconnect();
  this->SplitRecordTimer->stop();

  // Display a feedback message to the user when the recording is stopped
  QMessageBox stopRecordMsg;
  const QString txt = "Stream data have been saved in the directory:\n\n";
  stopRecordMsg.setWindowTitle("Stop stream recording");
  stopRecordMsg.setText(txt + this->CurrentRecorder->recordingFilename());
  stopRecordMsg.setStandardButtons(QMessageBox::Ok);
  stopRecordMsg.exec();

  Q_EMIT this->recording(false);
}

//-----------------------------------------------------------------------------
void lqStreamRecordController::onSourceRemoved(pqPipelineSource* src)
{
  auto toRemove = [&src](pqPipelineSource* activeSource)
  {
    return activeSource->getProxy()->GetGlobalID() == src->getProxy()->GetGlobalID(); // remove
  };
  auto newEnd = std::remove_if(
    this->ActiveRecordingSources.begin(), this->ActiveRecordingSources.end(), toRemove);
  this->ActiveRecordingSources.erase(newEnd, this->ActiveRecordingSources.end());

  if (this->IsRecording && this->ActiveRecordingSources.isEmpty())
  {
    this->stopRecording();
  }
}
