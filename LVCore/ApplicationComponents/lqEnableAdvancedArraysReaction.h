#ifndef LqEnableAdvancedArraysReaction_h
#define LqEnableAdvancedArraysReaction_h

#include <pqReaction.h>

#include "lqapplicationcomponents_export.h"

class pqPipelineSource;
/**
* @ingroup Reactions
* Reaction to enable and disable advanced arrays
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqEnableAdvancedArraysReaction : public pqReaction
{
    Q_OBJECT
    typedef pqReaction Superclass;

public:
  lqEnableAdvancedArraysReaction(QAction* action);

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
  Q_DISABLE_COPY(lqEnableAdvancedArraysReaction)

  void updateIcon(bool setEnable);
};

#endif // LqEnableAdvancedArraysReaction_h
