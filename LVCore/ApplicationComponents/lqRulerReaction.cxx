#include "lqRulerReaction.h"

#include <QMouseEvent>
#include <QtDebug>

#include <pqQVTKWidget.h>
#include <pqRenderView.h>
#include <pqSMAdaptor.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkRenderer.h>

//-----------------------------------------------------------------------------
lqRulerReaction::lqRulerReaction(QAction *parent, pqRenderView *v, lqRulerReaction::Mode m)
  : pqReaction(parent)
{
  Q_ASSERT(view);

  this->view = v;
  this->mode = m;

  QString toolTipText;
  if (mode == Mode::BETWEEN_2D_POINTS)
  {
    toolTipText = "Measure the distance (in meters) between two screen points.          \
                  <br/>To draw a ruler line, click this button, then hold down          \
                  the <b>Ctrl key</b> and the <b>left button</b> of your mouse.         \
                  <br/><u>This feature is only available in orthogonal projection.</u>";
  }
  else
  {
    toolTipText = "Measure the distance (in meters) between two 3D points.          \
                  <br/>To draw a ruler line, click this button, then hold down      \
                  the <b>Ctrl key</b> and the <b>left button</b> of your mouse.";
  }
  this->parentAction()->setToolTip(toolTipText);

  // Create a ruler and hide it for now
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  this->distanceWidgetRepresentation = proxyManager->NewProxy("representations", "DistanceWidgetRepresentation");
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->view->getProxy()->GetProperty("Representations"));
  pp->AddProxy(this->distanceWidgetRepresentation);
  this->displayRuler(false);

  if (mode == Mode::BETWEEN_2D_POINTS)
  {
    // The Ui need to be update, each time the ParalllelProjection property is Modified
    this->connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->connection->Connect(
      this->view->getProxy()->GetProperty("CameraParallelProjection"), vtkCommand::ModifiedEvent, this, SLOT(onTriggered()));
  }
  this->updateUI();
}

//-----------------------------------------------------------------------------
void lqRulerReaction::onTriggered()
{
  if (!view)
  {
    qCritical() << "No View for lqRulerReaction:";
    return;
  }

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
  if (!view)
  {
    qCritical() << "No View for lqRulerReaction:";
    return;
  }

  auto eventButton = event->buttons();
  auto eventModifier = event->modifiers();
  auto interactor = this->view->getRenderViewProxy()->GetInteractor();

  // Helper lambda to update the ruler widget coordinates points
  auto updateRulerPointWithMousePosition = [&](char* pointPropertyName)
  {
    auto point = this->get3DPoint(event->pos());
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
QList<QVariant> lqRulerReaction::get3DPoint(QPoint coord)
{
  if (!view)
  {
    qCritical() << "No View for lqRulerReaction:";
    return QList<QVariant>();
  }

  vtkSMRenderViewProxy* viewProxy = view->getRenderViewProxy();
  if (!viewProxy)
    return QList<QVariant>();

  auto viewSize = pqSMAdaptor::getMultipleElementProperty(this->view->getProxy()->GetProperty("ViewSize"));
  int windowHeight = viewSize[1].toInt();

  double position[3];

  if (mode == Mode::BETWEEN_2D_POINTS)
  {
    double midPlaneDistance = 0.5;
    double display3DPoint[3] = {coord.x(), windowHeight - coord.y(), midPlaneDistance};
    vtkRenderer* renderer = viewProxy->GetRenderer();
    renderer->SetDisplayPoint(display3DPoint);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(position);
  }
  else
  {
    int display3DPoint[2] = {coord.x(), windowHeight - coord.y()};
    if (!viewProxy->ConvertDisplayToPointOnSurface(display3DPoint, position, true))
    {
      return QList<QVariant>();
    }
  }

  QList<QVariant> point3d;
  point3d << position[0] << position[1] << position[2];

  return point3d;
}

//-----------------------------------------------------------------------------
void lqRulerReaction::displayRuler(bool value)
{
  if (!view)
  {
    qCritical() << "No View for lqRulerReaction:";
    return;
  }

  pqSMAdaptor::setElementProperty(this->distanceWidgetRepresentation->GetProperty("Visibility"), value);
  this->distanceWidgetRepresentation->UpdateVTKObjects();
  this->view->render();
}

//-----------------------------------------------------------------------------
void lqRulerReaction::updateUI()
{
  if (!view)
  {
    qCritical() << "No View for lqRulerReaction:";
    return;
  }

  if (mode == Mode::BETWEEN_2D_POINTS)
  {
    bool mode = pqSMAdaptor::getElementProperty(this->view->getProxy()->GetProperty("CameraParallelProjection")).toBool();
    this->parentAction()->setEnabled(mode);
    if (!mode)
    {
      this->parentAction()->setChecked(false);
    }
  }
}
