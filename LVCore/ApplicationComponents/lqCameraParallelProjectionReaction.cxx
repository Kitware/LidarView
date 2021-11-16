#include "lqCameraParallelProjectionReaction.h"

#include <QtDebug>

#include <pqActiveObjects.h>
#include <pqRenderView.h>
#include <pqSMAdaptor.h>
#include <vtkCommand.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkSMProxyProperty.h>

#include "lqapplicationcomponents_export.h"

lqCameraParallelProjectionReaction::lqCameraParallelProjectionReaction(QAction *parent)
  : pqReaction(parent)
{
  this->parentAction()->setToolTip("Toggle between projective and orthogonal view");

  // Connect Views
  QObject::connect(
      &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
      this, SLOT(onViewChanged(pqView*))
  );

  // Update for current View if any
  this->onViewChanged(pqActiveObjects::instance().activeView());
}

//-----------------------------------------------------------------------------
void lqCameraParallelProjectionReaction::onViewChanged(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  // Disable if not a render view
  this->parentAction()->setEnabled(static_cast<bool>(rview));

  // Update UI if it is a render view
  if (rview)
  {
    this->updateUI(rview);
  }
}

//-----------------------------------------------------------------------------
void lqCameraParallelProjectionReaction::onTriggered()
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  // No Active View
  if (!rview)
  {
    return;
  }

  // switch mode
  vtkSMProperty* property = rview->getProxy()->GetProperty("CameraParallelProjection");
  bool mode = !pqSMAdaptor::getElementProperty(property).toBool();
  pqSMAdaptor::setElementProperty(property, mode);
  rview->getProxy()->UpdateVTKObjects();
  rview->render();
  // no need to call updateUi here as the function will be trigger anyways thanks to the connection.
}

//-----------------------------------------------------------------------------
void lqCameraParallelProjectionReaction::updateUI(pqRenderView* view)
{
  // Not a RenderView
  if (!view) { return; }

  vtkSMProperty* property = view->getProxy()->GetProperty("CameraParallelProjection");
  bool mode = pqSMAdaptor::getElementProperty(property).toBool();
  this->parentAction()->setChecked(mode);
  QIcon icon = mode ? QIcon(":/lqResources/Icons/lqViewOrtho.png") : QIcon(":/lqResources/Icons/lqViewPerspective.png");
  this->parentAction()->setIcon(icon);
}
