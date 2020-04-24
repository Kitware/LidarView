#ifndef LQSTREAMRECORDREACTION_H
#define LQSTREAMRECORDREACTION_H

#include "pqReaction.h"

/**
* @ingroup Reactions
* Reaction to record stream data in a pcap file
*/
class lqStreamRecordReaction : public pqReaction
{
    Q_OBJECT
    typedef pqReaction Superclass;

public:
  lqStreamRecordReaction(QAction* action);

public slots:
  /**
  * Called when the action is triggered.
  */
  void actionTriggered();

private:
  Q_DISABLE_COPY(lqStreamRecordReaction)

  bool isRecording = false;
};

#endif // LQSTREAMRECORDREACTION_H
