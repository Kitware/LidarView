#include "lqRulerReaction.h"

#include <QtDebug>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqRenderView.h>
#include <pqSMAdaptor.h>
#include <pqServerManagerModel.h>
#include <pqView.h>
#include <vtkCallbackCommand.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderViewProxy.h>

//-----------------------------------------------------------------------------
void lqRulerReaction::mousePressCallback(
        vtkObject* caller,
        unsigned long vtkNotUsed(eventId),
        void* clientData,
        void* vtkNotUsed(callData))
{


  // Get Mouse Pos
  vtkRenderWindowInteractor* iren = static_cast<vtkRenderWindowInteractor*>(caller);
  int mousepos[2];
  iren->GetEventPosition(mousepos);

  // Get Reaction
  lqRulerReaction* rulerReact = reinterpret_cast<lqRulerReaction*>(clientData);
  if(!rulerReact)
  {
    qCritical() << "Null lqRulerReaction in Ruler callback";
  }

  // if not enabled return
  if(!rulerReact->isEnabled())
  {
    return;
  }

  // if no control key return
  if(!iren->GetControlKey())
  {
    return;
  }

  // Helper lambda to update the ruler widget coordinates points
  auto updateRulerPointWithMousePosition = [&](const char* pointPropertyName, const QPoint& coord)
  {
    auto point = rulerReact->get3DPoint(coord);
    pqSMAdaptor::setMultipleElementProperty(rulerReact->dwr->GetProperty(pointPropertyName), point);
    rulerReact->dwr->UpdateVTKObjects();
    rulerReact->view->getProxy()->UpdateVTKObjects();
    rulerReact->view->render();
  };

  QPoint mpos{mousepos[0], mousepos[1]};
  if(!rulerReact->started)
  {
    updateRulerPointWithMousePosition("Point1WorldPosition",mpos);
    updateRulerPointWithMousePosition("Point2WorldPosition",mpos);
  }else{
    updateRulerPointWithMousePosition("Point2WorldPosition",mpos);
  }
  rulerReact->started = !rulerReact->started;

}

//-----------------------------------------------------------------------------
lqRulerReaction::lqRulerReaction(QAction *parent, lqRulerReaction::Mode m)
  : pqReaction(parent), mode(m), view(nullptr), dwr(nullptr)
{

  // Connect View changes
  QObject::connect(
      &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
      this, SLOT(onViewChanged(pqView*))
  );
  // Update for current View if any
  this->onViewChanged(pqActiveObjects::instance().activeView());

  // Connect to View destruction
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
      smmodel, SIGNAL(preViewRemoved(pqView*)),
      this, SLOT(onViewRemoved(pqView*))
  );

  // Prevent 2D usage when CameraParallelProjection is off
  if (this->mode == Mode::BETWEEN_2D_POINTS)
  {
    // The Ui needs to be updated, each time the ParalllelProjection property is modified
    this->connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->connection->Connect(
      this->view->getProxy()->GetProperty("CameraParallelProjection"), vtkCommand::ModifiedEvent, this, SLOT(onTriggered()));
  }
  this->onTriggered(); // Update

  // Set Tooltip
  QString toolTipText;
  if (this->mode == Mode::BETWEEN_2D_POINTS)
  {
    toolTipText = "Measure the distance (in meters) between two screen points.      \
                  <br/>To draw a ruler line, click this button, then hold down      \
                  the <b>Ctrl key</b> and the <b>left button</b> of your mouse.     \
                  <br/><u>This feature is only available in orthogonal projection.</u>";
  }
  else
  {
    toolTipText = "Measure the distance (in meters) between two 3D points.          \
                  <br/>To draw a ruler line, click this button, then hold down      \
                  the <b>Ctrl key</b> and the <b>left button</b> of your mouse.";
  }
  this->parentAction()->setToolTip(toolTipText);

  // Set State
  this->parentAction()->setCheckable(true);
  this->parentAction()->setChecked(false);

  // Update UI for good measure
  this->updateUI();


}

//-----------------------------------------------------------------------------
void lqRulerReaction::onViewChanged(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);

  // Not a Render View selected
  if(!rview)
  {
    return;
  }

  // Did switch current renderView
  if (this->view == nullptr || this->view != view)
  {
    // Destroy old stuff if any
    this->destroyState();

    // Save Active View
    this->setView(rview);

    // Create associated dwr
    this->createDWR();
  }

  // Update Button / Ruler
  this->onTriggered();

  // Update UI
  this->updateUI();

}

//-----------------------------------------------------------------------------
void lqRulerReaction::onViewRemoved(pqView* view)
{
  // Clear State if the tracked renderview is removed
  if (this->view != nullptr && this->view == view){
    this->destroyState();
  }
}

//-----------------------------------------------------------------------------
void lqRulerReaction::onTriggered()
{
  // Return if no view
  if (!this->view)
  {
    return;
  }

  // Disable If not in Ortho mode
  if (this->mode == Mode::BETWEEN_2D_POINTS)
  {
    bool cameraParallelProjection = pqSMAdaptor::getElementProperty(this->view->getProxy()->GetProperty("CameraParallelProjection")).toBool();
    this->parentAction()->setEnabled(cameraParallelProjection);
    if (!cameraParallelProjection)
    {
      this->parentAction()->setChecked(false);
    }
  }

  // Display Ruler
  this->displayRuler(this->isEnabled());
}

//-----------------------------------------------------------------------------
void lqRulerReaction::setView(pqRenderView* rview)
{
  this->view = rview;

  // Set Mouse callback
  mouseCC = vtkCallbackCommand::New(); // Mouse press callback
  mouseCC->SetClientData(this);
  mouseCC->SetCallback(lqRulerReaction::mousePressCallback);
  vtkSMViewProxy::SafeDownCast(this->view->getProxy())->GetRenderWindow()->GetInteractor()->AddObserver(
    vtkCommand::LeftButtonPressEvent, mouseCC
  );

}

//-----------------------------------------------------------------------------
void lqRulerReaction::createDWR()
{
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  this->dwr = proxyManager->NewProxy("representations", "DistanceWidgetRepresentation");
  vtkSMProxyProperty::SafeDownCast(this->view->getProxy()->GetProperty("Representations"))->AddProxy(this->dwr);
}

//-----------------------------------------------------------------------------
void lqRulerReaction::destroyState()
{
  // Remove View / DWR if any
  if(this->view)
  {
    vtkSMProxyProperty::SafeDownCast(this->view->getProxy()->GetProperty("Representations"))->RemoveProxy(this->dwr);
    this->dwr->Delete();
    this->dwr  = nullptr; // TODO USE smarpointer ? e.g Catalyst/vtkCPProcessor.cxx NewProxy

    this->view->getProxy()->UpdateVTKObjects();
    this->view->render();
    this->view = nullptr;
  }

  // Reset Callback Command
  if(mouseCC)
  {
    mouseCC->SetClientData(nullptr);
    mouseCC->SetCallback(nullptr);
    mouseCC->Delete();
  }

  // Reset 'started' state
  this->started = false;
}

//-----------------------------------------------------------------------------
void lqRulerReaction::updateUI()
{
  if(this->view){
    this->view->getProxy()->UpdateVTKObjects();
    this->view->render();
  }
}

//-----------------------------------------------------------------------------
void lqRulerReaction::displayRuler(bool value)
{
  if(this->dwr)
  {
    pqSMAdaptor::setElementProperty(this->dwr->GetProperty("Visibility"), value);
    this->dwr->UpdateVTKObjects();
  }

  if(this->view)
  {
    this->view->getProxy()->UpdateVTKObjects();
    this->view->render();
  }
}

//-----------------------------------------------------------------------------
bool lqRulerReaction::isEnabled()
{
  return this->parentAction()->isChecked();
};

//-----------------------------------------------------------------------------
QList<QVariant> lqRulerReaction::get3DPoint(QPoint coord)
{

  if (!this->view)
  {
    qCritical() << "lqRulerReaction: get3DPoint has null view";
    return QList<QVariant>();
  }

  vtkSMRenderViewProxy* viewProxy = this->view->getRenderViewProxy();
  if (!viewProxy)
  {
    qCritical() << "qRulerReaction: get3DPoint view has null proxy";
    return QList<QVariant>();
  }

  // Was required for origin changes
  //auto viewSize = pqSMAdaptor::getMultipleElementProperty(this->view->getProxy()->GetProperty("ViewSize"));

  double position[3];

  if (this->mode == Mode::BETWEEN_2D_POINTS)
  {
    // in the Mode::BETWEEN_2D_POINTS, we will create a 3D points that project to the mouse coordinate
    // To do so we create a point express in display (or screen) coordinates and let the render convert
    // the coordinate to world.
    double midPlaneDistance = 0.5;
    // VTK points have bottom left origin
    double display3DPoint[3] = {static_cast<double>(coord.x()), static_cast<double>(coord.y()), midPlaneDistance};
    vtkRenderer* renderer = viewProxy->GetRenderer();
    renderer->SetDisplayPoint(display3DPoint);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(position);
  }
  else
  {
    // int the Mode::BETWEEN_3D_POINTS, we will lock for an existing 3D points that project to the mouse coordinate
    // For more information look at the vtkSMRenderViewProxy::ConvertDisplayToPointOnSurface function
    int display3DPoint[2] = {coord.x(), coord.y()};
    if (!viewProxy->ConvertDisplayToPointOnSurface(display3DPoint, position, true))
    {
      return QList<QVariant>();
    }
  }

  QList<QVariant> point3d;
  point3d << position[0] << position[1] << position[2];

  return point3d;
}
