#include "lqCameraParallelProjectionReaction.h"

#include <QtDebug>

#include <pqRenderView.h>
#include <pqSMAdaptor.h>
#include <vtkCommand.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkSMProxyProperty.h>


lqCameraParallelProjectionReaction::lqCameraParallelProjectionReaction(QAction *parent,  pqRenderView* v)
  : pqReaction(parent)
{
  Q_ASSERT(view);

  this->view = v;
  this->parentAction()->setToolTip("Toggle between projective and orthogonal view");
  // update the button state is coherent with the camera projection mode
  updateUI();

  // The Ui need to be update, each time the ParalllelProjection property is Modified
  this->connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->connection->Connect(
    this->view->getProxy()->GetProperty("CameraParallelProjection"), vtkCommand::ModifiedEvent, this, SLOT(updateUI()));
}

void lqCameraParallelProjectionReaction::onTriggered()
{
  if (!view)
  {
    qCritical() << "No View for lqCameraParallelProjectionReaction:";
    return;
  }

  // switch mode
  vtkSMProperty* property = this->view->getProxy()->GetProperty("CameraParallelProjection");
  bool mode = !pqSMAdaptor::getElementProperty(property).toBool();
  pqSMAdaptor::setElementProperty(property, mode);
  this->view->getProxy()->UpdateVTKObjects();
  // no need to call updateUi here as the function will be trigger anyways thanks to the connection.
}

void lqCameraParallelProjectionReaction::updateUI()
{
  if (!view)
  {
    qCritical() << "No View for lqCameraParallelProjectionReaction:";
    return;
  }

  bool mode = pqSMAdaptor::getElementProperty(this->view->getProxy()->GetProperty("CameraParallelProjection")).toBool();
  this->parentAction()->setChecked(mode);
  this->view->render();
}
