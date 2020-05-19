#include "lqStreamRecordReaction.h"

#include <QFileDialog>
#include <QString>

#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqPipelineSource.h>
#include <vtkSMProxy.h>

#include "lqStreamRecordDialog.h"
#include "vtkLidarStream.h"
//-----------------------------------------------------------------------------
lqStreamRecordReaction::lqStreamRecordReaction(QAction *action)
  : Superclass(action)
{
}

//-----------------------------------------------------------------------------
void lqStreamRecordReaction::onTriggered()
{
  if (this->isRecording)
  {
    vtkStream::StopRecording();
    this->isRecording = false;
    this->parentAction()->setToolTip("Start Recording Stream Data");
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
        this->isRecording = true;
        lidarName = src->getSMName();
        lidar = tmp;
        break;
      }
    }

    assert(lidar);
    assert(lidar->GetInterpreter());

    lqStreamRecordDialog dialog(nullptr, lidarName);
    if (dialog.exec())
    {
      this->parentAction()->setToolTip("Stop Recording Stream Data");
      vtkStream::StartRecording(dialog.recordingFile().toStdString());
      QFile::copy(QString::fromStdString(lidar->GetLidarInterpreter()->GetCalibrationFileName()),
                  dialog.calibrationFile());
    }
  }
}
  }
}
