#ifndef LQOPENPCAPREACTION_H
#define LQOPENPCAPREACTION_H

#include "applicationui_export.h"

#include "pqReaction.h"

#include <vtkObject.h>
/**
* @ingroup Reactions
* Reaction to open a pcap
*/
class APPLICATIONUI_EXPORT lqOpenPcapReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqOpenPcapReaction(QAction* action);
  static void createSourceFromFile(QString fileName);

protected:
  /// Called when the action is triggered.
  void onTriggered() override;

  static void onProgressEvent(vtkObject* caller, unsigned long, void*);

private:

  class vtkObserver;
  Q_DISABLE_COPY(lqOpenPcapReaction)
};

#endif // LQOPENPCAPREACTION_H
