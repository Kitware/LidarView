#ifndef LQRULERREACTION_H
#define LQRULERREACTION_H

#include <pqReaction.h>
#include <vtkSmartPointer.h>

class pqRenderView;
class QMouseEvent;
class vtkEventQtSlotConnect;
class vtkSMProxy;

/**
 * @ingroup Reactions
 * Reaction to use a ruler to measure the distance between 2 points in a scene.
 */
class lqRulerReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqRulerReaction(QAction* parent, pqRenderView* view);

protected slots:
  void updateUI();

  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

  /**
   * Update the ruler widget cordinate based on the mouse event.
   */
  void onMouseEvent(QMouseEvent* event);
private:
  QList<QVariant> getPointFromCoordinate(QPoint coord, double midPlaneDistance = 0.5);
  void displayRuler(bool value);

  Q_DISABLE_COPY(lqRulerReaction)
  pqRenderView* view;
  bool mousePressed = true;

  vtkSMProxy* distanceWidgetRepresentation;
  vtkSmartPointer<vtkEventQtSlotConnect> connection;
};

#endif // LQRULERREACTION_H
