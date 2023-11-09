/*=========================================================================

  Program: LidarView
  Module:  lqCameraParallelProjectionReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqCameraParallelProjectionReaction.h"

#include <QtDebug>

#include <pqActiveObjects.h>
#include <pqRenderView.h>
#include <pqSMAdaptor.h>
#include <vtkCommand.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkSMProxyProperty.h>

lqCameraParallelProjectionReaction::lqCameraParallelProjectionReaction(QAction* parent)
  : pqReaction(parent)
{
  this->parentAction()->setToolTip("Toggle between projective and orthogonal view");

  // Connect Views
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(onViewChanged(pqView*)));
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewUpdated()), this, SLOT(updateUI()));

  // Update for current View if any
  this->onViewChanged(pqActiveObjects::instance().activeView());
}

//-----------------------------------------------------------------------------
void lqCameraParallelProjectionReaction::onViewChanged(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  // Disable if not a render view
  this->parentAction()->setEnabled(static_cast<bool>(rview));
  this->updateUI();
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
  this->updateUI();
}

//-----------------------------------------------------------------------------
void lqCameraParallelProjectionReaction::updateUI()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  // No Active View
  if (!view)
  {
    return;
  }

  vtkSMProperty* property = view->getProxy()->GetProperty("CameraParallelProjection");
  bool mode = pqSMAdaptor::getElementProperty(property).toBool();
  this->parentAction()->setChecked(mode);
  QIcon icon = mode ? QIcon(":/lqResources/Icons/lqViewOrtho.png")
                    : QIcon(":/lqResources/Icons/lqViewPerspective.png");
  this->parentAction()->setIcon(icon);
}
