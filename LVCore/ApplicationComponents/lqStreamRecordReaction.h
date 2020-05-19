#ifndef LQSTREAMRECORDREACTION_H
#define LQSTREAMRECORDREACTION_H

#include "pqReaction.h"

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
  lqStreamRecordReaction(QAction* action);

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
};

#endif // LQSTREAMRECORDREACTION_H
