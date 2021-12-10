#ifndef LQRULERREACTION_H
#define LQRULERREACTION_H

#include <pqReaction.h>
#include <vtkSmartPointer.h>

#include "lqapplicationcomponents_export.h"

class QMouseEvent;
class pqRenderView;
class pqView;
class vtkCallbackCommand;
class vtkObject;
class vtkSMProxy;
class vtkEventQtSlotConnect;

/**
 * @ingroup Reactions
 * Reaction to use a ruler to measure the distance between 2 points in a scene.
 *
 * These points could either be 2D in case the view is orthographic or 3D
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqRulerReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  enum Mode
  {
    BETWEEN_2D_POINTS,
    BETWEEN_3D_POINTS,
  };
  lqRulerReaction(QAction* parent, lqRulerReaction::Mode mode);

protected slots:

  /**
  * Called when activeView changed.
  */
  void onViewChanged(pqView* view);

  /**
  * Called when activeView changed.
  */
  void onViewRemoved(pqView* view);

  /**
  * Called when the action is triggered / when projection changes
  */
  void onTriggered() override;

private:

  // Mouse Click Callback
  static void mousePressCallback(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // Internal Helpers
  void setView(pqRenderView* rview); // Set the new Rview
  void createDWR();                  // Create new distanceWidgetRepresentation
  void destroyState();               // Clear Saved View and dwr
  void displayRuler(bool value);     // Change dwr visibility

  void updateUI();  // Update UI according to state

  bool isEnabled(); // Is action checked

  QList<QVariant> get3DPoint(QPoint);

  Q_DISABLE_COPY(lqRulerReaction)

  // Creation state
  const Mode mode;
  vtkSmartPointer<vtkEventQtSlotConnect> connection; // Used to listen for projection state changes

  // Tracked State
  pqRenderView* view; // Current active View
  vtkSMProxy* dwr;    // Current active View's distanceWidgetRepresentation
  vtkSmartPointer<vtkCallbackCommand> mouseCC; // Mouse Callback command
  bool started = false; // Set to true when ruler has been clicked once

};

#endif // LQRULERREACTION_H
