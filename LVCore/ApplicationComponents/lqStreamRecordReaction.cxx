#include "lqStreamRecordReaction.h"

#include <QFileDialog>
#include <QMessageBox>

#include <pqApplicationCore.h>
#include <pqFileDialog.h>
#include <pqServerManagerModel.h>
#include <pqPipelineSource.h>
#include <vtkSMProxy.h>

#include "lqStreamRecordDialog.h"
#include "vtkLidarStream.h"

namespace {
bool isStream(pqPipelineSource* src)
{
  return ( src != nullptr
            && src->getProxy() != nullptr
            && src->getProxy()->GetClientSideObject()->IsA("vtkLidarStream"));
}

bool isLastStream(pqPipelineSource* src)
{
  if (!isStream(src)) return false;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    if (isStream(src))
    {
      return false;
    }
  }
  return true;
}
}

//-----------------------------------------------------------------------------
lqStreamRecordReaction::lqStreamRecordReaction(QAction *action, bool displayStopMessage, bool useAdvancedDialog)
  : Superclass(action)
{
  this->parentAction()->setEnabled(false);
  this->useAdvancedDialog = useAdvancedDialog;
  this->displayStopMessage = displayStopMessage;
  auto* core = pqApplicationCore::instance();

  pqServerManagerModel* smmodel = core->getServerManagerModel();
  this->connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onSourceAdded(pqPipelineSource*)));
  this->connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(onSourceRemoved(pqPipelineSource*)));

  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    this->onSourceAdded(src);

  this->recordingFilename = "";
  this->parentAction()->setCheckable(true);
}

//-----------------------------------------------------------------------------
void lqStreamRecordReaction::onTriggered()
{
  if (vtkStream::IsRecording())
  {
    vtkStream::StopRecording();
    this->parentAction()->setToolTip("Start Recording Stream Data");
    this->parentAction()->setChecked(false);

    if(this->displayStopMessage)
    {
      // Display a feedback message to the user when the recording is stopped
      QMessageBox stopRecordMsg;
      const QString txt = "Stop Recording.\nStream data have been saved in the file ";
      stopRecordMsg.setText(txt + this->recordingFilename);
      stopRecordMsg.setStandardButtons(QMessageBox::Ok);
      stopRecordMsg.exec();
    }
  }
  else
  {
    // Get the first lidar, as for now this function doesn't handle multiple lidar.
    // When this code need to be upgrade for multiple lidar so that either
    // a meta calibration file or individual calibration file are saved.
    vtkLidarStream* lidar = nullptr;
    QString lidarName;
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    {
      auto* tmp = dynamic_cast<vtkLidarStream*> (src->getProxy()->GetClientSideObject());
      if (tmp)
      {
        lidarName = src->getSMName();
        lidar = tmp;
        break;
      }
    }

    assert(lidar);
    assert(lidar->GetInterpreter());

    if(useAdvancedDialog)
    {
      lqStreamRecordDialog dialog(nullptr, lidarName);
      if (dialog.exec())
      {
        this->parentAction()->setToolTip("Stop Recording Stream Data");
        this->recordingFilename = dialog.recordingFile();
        QFile::copy(QString::fromStdString(lidar->GetLidarInterpreter()->GetCalibrationFileName()),
                    dialog.calibrationFile());
      }
    }
    else
    {
      this->recordingFilename = QFileDialog::getSaveFileName(nullptr,
                      QString("Record File:"), QString(""), QString("PCAP (*.pcap)"));
    }
    if (this->recordingFilename != "")
    {
      vtkStream::StartRecording(this->recordingFilename.toStdString());
      this->parentAction()->setChecked(true);
    }

  }
}

//-----------------------------------------------------------------------------
void lqStreamRecordReaction::onSourceAdded(pqPipelineSource *src)
{
  if (!this->parentAction()->isEnabled() && isStream(src))
  {
    this->parentAction()->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
void lqStreamRecordReaction::onSourceRemoved(pqPipelineSource *src)
{
  if (this->parentAction()->isEnabled() && isLastStream(src))
  {
    this->parentAction()->setEnabled(false);
  }
}
