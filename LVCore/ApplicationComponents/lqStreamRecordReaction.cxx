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
#include "PacketFileWriter.h"

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
  : Superclass(action),
    Settings(pqApplicationCore::instance()->settings()),
    isRecording(false)
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
  if(this->isRecording)
  {
    this->StopRecordingReaction();
  }
  else
  {
    this->StartRecordingReaction();
  }
}

//-----------------------------------------------------------------------------
void lqStreamRecordReaction::StartRecordingReaction()
{
  // Get the first lidar, as for now this function doesn't handle multiple lidar.
  // When this code need to be upgrade for multiple lidar so that either
  // a meta calibration file or individual calibration file are saved.
  vtkLidarStream* lidar = nullptr;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    auto* tmp = dynamic_cast<vtkLidarStream*> (src->getProxy()->GetClientSideObject());
    if (tmp)
    {
      lidar = tmp;
      break;
    }
  }

  assert(lidar);
  vtkLidarPacketInterpreter* interpreter = lidar->GetLidarInterpreter();
  assert(interpreter);

  QString defaultFileName = QString::fromStdString(interpreter->GetDefaultRecordFileName() + ".pcap");
  if(useAdvancedDialog)
  {
    lqStreamRecordDialog dialog(nullptr, defaultFileName);
    if ( dialog.exec() == QDialog::Rejected )
    {
      // Cancel
      return;
    }
    // Get User input Filename
    this->recordingFilename = dialog.recordingFile();
    QFile::copy(QString::fromStdString(interpreter->GetCalibrationFileName()),
                dialog.calibrationFile());
  }
  else
  {
    QString PreviousPath = QString(this->Settings->value("LidarPlugin/RecordReaction/DefaultFolder").toString());
    defaultFileName = PreviousPath + "/" + defaultFileName;

    this->recordingFilename = QFileDialog::getSaveFileName(nullptr,
                    QString("Record File:"), defaultFileName, QString("PCAP (*.pcap)"));
  }
  if (this->recordingFilename == "")
  {
    std::cout << "Recording Filename is empty, aborting." << std::endl;
    return;
  }

  // Create common writer Thread
  std::shared_ptr<PacketFileWriter> writer = std::make_shared<PacketFileWriter>();
  // Tell vtkStreams to Start Recording
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    auto* stream = dynamic_cast<vtkStream*> (src->getProxy()->GetClientSideObject());
    if(!stream){continue;}
    stream->StartRecording( // Multiple start is okay, it detects it
      this->recordingFilename.toStdString(),
      writer
    );
  }

  // Update state
  this->isRecording = true;
  this->parentAction()->setToolTip("Stop Recording Stream Data");
  this->parentAction()->setChecked(true);

  // Save the path where the pcap is saved
  QFileInfo fileInfo(this->recordingFilename);
  this->Settings->setValue("LidarPlugin/RecordReaction/DefaultFolder", fileInfo.absolutePath());

}

//-----------------------------------------------------------------------------
void lqStreamRecordReaction::StopRecordingReaction()
{
  // Tell vtkStreams to Stop Recording
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    auto* stream = dynamic_cast<vtkStream*> (src->getProxy()->GetClientSideObject());
    if(!stream){continue;}
    stream->StopRecording();
  }

  // Update State
  this->isRecording = false;
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
    if (this->isRecording)
    {
      this->StopRecordingReaction();
    }
  }
}
