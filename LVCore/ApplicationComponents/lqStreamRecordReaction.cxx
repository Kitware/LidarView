#include "lqStreamRecordReaction.h"

#include "vtkStream.h"
#include "pqFileDialog.h"

#include <QString>

//-----------------------------------------------------------------------------
lqStreamRecordReaction::lqStreamRecordReaction(QAction *action)
  : Superclass(action)
{
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
}

//-----------------------------------------------------------------------------
void lqStreamRecordReaction::actionTriggered()
{
  if (this->isRecording)
  {
    vtkStream::StopRecording();
    this->isRecording = false;
    this->parentAction()->setToolTip("Start Recording Stream Data");
  }
  else
  {
      std::string filename = pqFileDialog::getSaveFileName(
            nullptr, nullptr, tr("Record File:"), QString(), "*.pcap").toStdString();
      if (filename != "")
      {
        this->isRecording = true;
        this->parentAction()->setToolTip("Stop Recording Stream Data");
        vtkStream::StartRecording(filename);
      }
  }
}
