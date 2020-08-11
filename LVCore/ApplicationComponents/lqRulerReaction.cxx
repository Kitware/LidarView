#include "lqRulerReaction.h"

#include <QMouseEvent>

#include <pqQVTKWidget.h>
#include <pqRenderView.h>
#include <pqSMAdaptor.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkRenderer.h>

//-----------------------------------------------------------------------------
lqRulerReaction::lqRulerReaction(QAction *parent, pqRenderView *v)
  : pqReaction(parent)
{
  this->view = v;
  this->parentAction()->setToolTip("Measure the distance (in meters) between two screen points."
                                   "<br/>To draw a ruler line, click this button, then hold down"
                                   "the <b>Ctrl key</b> and the <b>left button</b> of your mouse."
                                   "<br/><u>This feature is only available in orthogonal projection.</u>");

  // Create a ruler and hide it for now
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  this->distanceWidgetRepresentation = proxyManager->NewProxy("representations", "DistanceWidgetRepresentation");
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->view->getProxy()->GetProperty("Representations"));
  pp->AddProxy(this->distanceWidgetRepresentation);
  this->displayRuler(false);

  // The Ui need to be update, each time the ParalllelProjection property is Modified
  this->connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->connection->Connect(
    this->view->getProxy()->GetProperty("CameraParallelProjection"), vtkCommand::ModifiedEvent, this, SLOT(onTriggered()));

  this->updateUI();
}

//-----------------------------------------------------------------------------
void lqRulerReaction::onTriggered()
{
  // need to be done first as the button status is used to know if the ruler is active or not.
  this->updateUI();

  pqQVTKWidget* v = dynamic_cast<pqQVTKWidget*>(this->view->widget());
  if (this->parentAction()->isChecked())
  {
    QObject::connect(v, &pqQVTKWidget::mouseEvent, this, &lqRulerReaction::onMouseEvent);
    this->mousePressed = false;
  }
  else
  {
    QObject::disconnect(v, &pqQVTKWidget::mouseEvent, this, &lqRulerReaction::onMouseEvent);
    this->displayRuler(false);
  }
}

//-----------------------------------------------------------------------------
void lqRulerReaction::onMouseEvent(QMouseEvent *event)
{
  auto eventButton = event->buttons();
  auto eventModifier = event->modifiers();
  auto interactor = this->view->getRenderViewProxy()->GetInteractor();

  // Helper lambda to update the ruler widget coordinates points
  auto updateRulerPointWithMousePosition = [&](char* pointPropertyName)
  {
    auto point = this->getPointFromCoordinate(event->pos());
    pqSMAdaptor::setMultipleElementProperty(this->distanceWidgetRepresentation->GetProperty(pointPropertyName), point);
    this->distanceWidgetRepresentation->UpdateVTKObjects();
  };

  if (eventButton == Qt::MouseButton::LeftButton)
  {
    if (!this->mousePressed)
    {
      if (eventModifier == Qt::Modifier::CTRL)
      {
        this->mousePressed = true;
        updateRulerPointWithMousePosition("Point1WorldPosition");
        interactor->Disable();
      }
    }
    else // mousePressed == true
    {
      updateRulerPointWithMousePosition("Point2WorldPosition");
      this->displayRuler(true);
    }
  }
  else if (eventButton==Qt::MouseButton::NoButton)
  {
    interactor->Enable();
    if (this->mousePressed)
    {
      this->mousePressed = false;
      updateRulerPointWithMousePosition("Point2WorldPosition");
      displayRuler(true);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> lqRulerReaction::getPointFromCoordinate(QPoint coord, double midPlaneDistance  )
{
  auto viewSize = pqSMAdaptor::getMultipleElementProperty(this->view->getProxy()->GetProperty("ViewSize"));
  int windowHeight = viewSize[1].toInt();
  double displayPoint[3] = {coord.x(), windowHeight - coord.y(), midPlaneDistance};
  vtkRenderer* renderer = this->view->getRenderViewProxy()->GetRenderer();
  renderer->SetDisplayPoint(displayPoint);
  renderer->DisplayToWorld();
  double* worldPoint = renderer->GetWorldPoint();
  QList<QVariant> point3d;
  point3d << worldPoint[0] << worldPoint[1] << worldPoint[2];
}

//-----------------------------------------------------------------------------
void lqRulerReaction::displayRuler(bool value)
{
  pqSMAdaptor::setElementProperty(this->distanceWidgetRepresentation->GetProperty("Visibility"), value);
  this->distanceWidgetRepresentation->UpdateVTKObjects();
  this->view->render();
}

//-----------------------------------------------------------------------------
void lqRulerReaction::updateUI()
{
  if (!view)
    return;
  bool mode = pqSMAdaptor::getElementProperty(this->view->getProxy()->GetProperty("CameraParallelProjection")).toBool();
  this->parentAction()->setEnabled(mode);
  if (!mode)
  {
    this->parentAction()->setChecked(false);
  }
}
