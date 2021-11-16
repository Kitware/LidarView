#ifndef LQCAMERAPARALLELPROJECTIONREACTION_H
#define LQCAMERAPARALLELPROJECTIONREACTION_H

#include <pqReaction.h>
#include <vtkSmartPointer.h>

#include "lqapplicationcomponents_export.h"

class pqView;
class pqRenderView;

/**
* @ingroup Reactions
* Reaction to change the camera projection type between perspective and parallel
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqCameraParallelProjectionReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqCameraParallelProjectionReaction(QAction* parent);

protected slots:

  /**
  * Called when activeView changed.
  */
  void onViewChanged(pqView* view);

  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

  /**
   * Update the button state according to the camera state.
   */
  void updateUI(pqRenderView* rview);

private:
  Q_DISABLE_COPY(lqCameraParallelProjectionReaction)
};

#endif // LQCAMERAPARALLELPROJECTIONREACTION_H
