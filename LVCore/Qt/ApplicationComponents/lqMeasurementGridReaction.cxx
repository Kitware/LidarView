/*=========================================================================

  Program: LidarView
  Module:  lqMeasurementGridReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqMeasurementGridReaction.h"

#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMViewProxy.h>

#include <QMenu>

#include <pqActiveObjects.h>
#include <pqRenderView.h>
#include <pqSettings.h>
#include <pqView.h>

#include "lqHelper.h"

namespace
{
bool isGridVisible(pqRenderView* view)
{
  double visibility;
  vtkSMProxy* lidarGridProxy = vtkSMPropertyHelper(view->getProxy(), "LidarGrid").GetAsProxy();
  vtkSMPropertyHelper(lidarGridProxy, "Visibility").Get(&visibility);
  return visibility;
}
}

//-----------------------------------------------------------------------------
lqMeasurementGridReaction::lqMeasurementGridReaction(QAction* parent)
  : Superclass(parent)
{
  this->connect(
    this->parentAction(), &QAction::toggled, lqMeasurementGridReaction::updateGridVisibility);
}

//-----------------------------------------------------------------------------
void lqMeasurementGridReaction::onRefreshButton()
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqRenderView* renderView = dynamic_cast<pqRenderView*>(view);
  if (renderView)
  {
    if (view->getProxy()->GetProperty("LidarGrid"))
    {
      double visibility = ::isGridVisible(renderView);
      this->parentAction()->setEnabled(true);
      this->parentAction()->setChecked(visibility);
    }
    else
    {
      this->parentAction()->setEnabled(false);
      this->parentAction()->setChecked(false);
    }
    this->parentAction()->blockSignals(false);
  }
}

//-----------------------------------------------------------------------------
void lqMeasurementGridReaction::updateGridVisibility(bool show)
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqRenderView* renderView = dynamic_cast<pqRenderView*>(view);
  if (renderView)
  {
    if (view->getProxy()->GetProperty("LidarGrid"))
    {
      vtkSMProxy* lidarGridProxy = vtkSMPropertyHelper(view->getProxy(), "LidarGrid").GetAsProxy();
      if (lidarGridProxy)
      {
        // If the state asked is already active it is probably
        // the other one needed.
        if (show == ::isGridVisible(renderView))
        {
          show = !show;
        }
        vtkSMPropertyHelper(lidarGridProxy, "Visibility").Set(show);
        lidarGridProxy->UpdateVTKObjects();
        view->render();

        // Save current state
        pqSettings* settings = pqApplicationCore::instance()->settings();
        settings->setValue("MeasurementGridState", show);
      }
    }
  }
}
