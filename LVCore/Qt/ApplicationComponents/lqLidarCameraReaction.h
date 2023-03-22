#ifndef lqLidarCameraReaction_h
#define lqLidarCameraReaction_h

#include <pqReaction.h>
#include <pqCameraReaction.h>
#include "lqApplicationComponentsModule.h"

class LQAPPLICATIONCOMPONENTS_EXPORT lqLidarCameraReaction : public pqCameraReaction
{
    Q_OBJECT
    typedef pqCameraReaction Superclass;

public:
  lqLidarCameraReaction(QAction* action,  pqCameraReaction::Mode mode);

public Q_SLOTS:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

  void ResetLidarPositiveX();
  void ResetLidarPositiveY();
  void ResetLidarPositiveZ();
  void ResetLidarNegativeX();
  void ResetLidarNegativeY();
  void ResetLidarNegativeZ();

private:
  Q_DISABLE_COPY(lqLidarCameraReaction)

  void ResetCenterOfRotationOnLidarCenter();

  pqCameraReaction::Mode ReactionMode;

};

#endif // lqLidarCameraReaction_h
