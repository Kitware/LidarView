#ifndef LQOPENSENSORREACTION_H
#define LQOPENSENSORREACTION_H

#include "lvApplicationComponentsModule.h"

#include "pqReaction.h"
#include "lqCalibrationDialog.h"

/**
 * @ingroup Reactions
 * Reaction to open a sensor stream
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqOpenSensorReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqOpenSensorReaction(QAction* action);
  static void createSensorStream(const lqCalibrationDialog& dialog);

protected:
  /// Called when the action is triggered.
  virtual void onTriggered() override;

private:
  Q_DISABLE_COPY(lqOpenSensorReaction)
};

#endif // LQOPENSENSORREACTION_H
