#ifndef LQCAMERAPARALLELPROJECTIONREACTION_H
#define LQCAMERAPARALLELPROJECTIONREACTION_H

#include <pqReaction.h>
#include <vtkSmartPointer.h>

#include "lqapplicationcomponents_export.h"

class pqRenderView;
class vtkEventQtSlotConnect;

/**
* @ingroup Reactions
* Reaction to change the camera projection type between perspective and parallel
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqCameraParallelProjectionReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqCameraParallelProjectionReaction(QAction* parent, pqRenderView* view);

protected slots:

  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

  /**
   * Update the button state according to the camera state.
   */
  void updateUI();

private:
  Q_DISABLE_COPY(lqCameraParallelProjectionReaction)
  pqRenderView* view;
  vtkSmartPointer<vtkEventQtSlotConnect> connection;
};

#endif // LQCAMERAPARALLELPROJECTIONREACTION_H
