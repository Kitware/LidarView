#ifndef LQSTREAMRECORDREACTION_H
#define LQSTREAMRECORDREACTION_H

#include <QString>

#include "pqReaction.h"
#include <pqSettings.h>

#include "lqapplicationcomponents_export.h"

class pqPipelineSource;
/**
* @ingroup Reactions
* Reaction to record stream data in a pcap file
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqStreamRecordReaction : public pqReaction
{
    Q_OBJECT
    typedef pqReaction Superclass;

public:
  lqStreamRecordReaction(QAction* action, bool displayStopMessage = true,
                         bool useAdvancedDialog = false);

public slots:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

  /**
   * Monitor the added/deleted source to enable/disable the QAction
   */
  void onSourceAdded(pqPipelineSource* src);
  void onSourceRemoved(pqPipelineSource* src);


private:
  Q_DISABLE_COPY(lqStreamRecordReaction)

  // When pressing the "record" button :
  // If useAdvancedDialog is false : the dialog is the classic one (the user just need to choose a recording pcap file)
  // If useAdvancedDialog is true : the dialog is the advanced one (to record the calibration file too)
  bool useAdvancedDialog;

  // Option to not display the message when recording is stopped
  bool displayStopMessage;

  // Filename where the stream data are saved
  QString recordingFilename;

  pqSettings* const Settings;
  
  // Actions when triggered :
  // Open pop up to choose file path where to save stream + start stream recording with vtkstream + change state for next trigger
  void StartRecordingReaction();
  // Stop stream recording + open pop up to warn user + change state for next trigger, (recording must have been previously started)
  void StopRecordingReaction();
  
  bool isRecording;
};

#endif // LQSTREAMRECORDREACTION_H
