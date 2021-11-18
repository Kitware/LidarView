#include "lqViewFrameActions.h"

#include <QList>

#include <pqRenderView.h>
#include <pqView.h>
#include <pqViewFrame.h>

#include <lqCameraParallelProjectionReaction.h>
#include <lqRulerReaction.h>

//-----------------------------------------------------------------------------
lqViewFrameActions::lqViewFrameActions(QObject* parent)
  :pqStandardViewFrameActionsImplementation(parent)
{
  // WIP we could take as optional argument the 2D/3D lqRulerReaction, in order to not instantiate multiple ones
  // and be able to have a general button in the App main UI
}

//-----------------------------------------------------------------------------
void lqViewFrameActions::frameConnected(pqViewFrame* frame, pqView* view)
{
  Q_ASSERT(frame != NULL);
  pqStandardViewFrameActionsImplementation::frameConnected(frame, view);
  frame->addTitleBarSeparator();
  if (pqRenderView* const render_view = qobject_cast<pqRenderView*>(view))
  {
    // Mutually exclusive 2D / 3D Selection
    QActionGroup* actionGroup = new QActionGroup(this);

    QAction* measurePtP = frame->addTitleBarAction(QIcon(":/lqResources/Icons/lqRuler3D.png"), "Tooltip overwrite by reaction");
    measurePtP->setObjectName("measurePointToPoint");
    new lqRulerReaction(measurePtP, lqRulerReaction::Mode::BETWEEN_3D_POINTS);
    actionGroup->addAction(measurePtP);

    QAction* measure = frame->addTitleBarAction(QIcon(":/lqResources/Icons/lqRuler2D.png"), "Tooltip overwrite by reaction");
    measure->setObjectName("measureinParallelePlane");
    new lqRulerReaction(measure   , lqRulerReaction::Mode::BETWEEN_2D_POINTS);
    actionGroup->addAction(measure);

    // Camera Projection Action
    QAction* cameraProjection = frame->addTitleBarAction(QIcon(":/lqResources/Icons/lqViewPerspective.png"), "Tooltip overwrite by reaction");
    cameraProjection->setObjectName("cameraProjection");
    new lqCameraParallelProjectionReaction(cameraProjection);
  }
}

//-----------------------------------------------------------------------------
bool lqViewFrameActions::isButtonVisible(const std::string& buttonName, pqView* vtkNotUsed(view) )
{
  QList<std::string> disabled_button {"SelectSurfaceCells",
                                      "SelectFrustumCells",
                                      "SelectPolygonSelectionCells",
                                      "SelectBlock",
                                      "InteractiveSelectSurfaceCells",
                                      "HoveringSurfaceCells"};
  if (disabled_button.contains(buttonName))
  {
    return false;
  }
  return true;
}
