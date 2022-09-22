#ifndef LQOPENSENSORREACTION_H
#define LQOPENSENSORREACTION_H

#include "applicationui_export.h"

#include "pqReaction.h"
#include "vvCalibrationDialog.h"

/**
 * @ingroup Reactions
 * Reaction to open a sensor stream
 */
class APPLICATIONUI_EXPORT lqOpenSensorReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqOpenSensorReaction(QAction* action);
  static void createSensorStream(const vvCalibrationDialog& dialog);

protected:
  /// Called when the action is triggered.
  virtual void onTriggered() override;

private:
  Q_DISABLE_COPY(lqOpenSensorReaction)
};

#endif // LQOPENSENSORREACTION_H
