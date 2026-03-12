/*=========================================================================

  Program:   LidarView
  Module:    lqLidarCorePluginManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLidarCorePluginManager.h"

#include <vtkSMPropertyHelper.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMViewProxy.h>

#include <QtDebug>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqPluginManager.h>
#include <pqRenderView.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqSettings.h>
#include <pqView.h>

//-----------------------------------------------------------------------------
lqLidarCorePluginManager::lqLidarCorePluginManager(QObject* p /*=0*/)
  : QObject(p)
{
  pqPluginManager* pluginManager = pqApplicationCore::instance()->getPluginManager();
  QObject::connect(pluginManager,
    &pqPluginManager::pluginsUpdated,
    this,
    &lqLidarCorePluginManager::onPluginLoaded);

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  // Needed for reset session
  QObject::connect(builder,
    &pqObjectBuilder::finishedAddingServer,
    this,
    &lqLidarCorePluginManager::onServerAdded);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, &pqServerManagerModel::viewAdded, this, &lqLidarCorePluginManager::onViewAdded);
}

//-----------------------------------------------------------------------------
lqLidarCorePluginManager::~lqLidarCorePluginManager() = default;

//-----------------------------------------------------------------------------
void lqLidarCorePluginManager::onPluginLoaded()
{
  static bool loaded = false;
  if (!loaded)
  {
    this->loadLidarRenderView();
    this->loadLidarPalette();
    loaded = true;
  }
}

//-----------------------------------------------------------------------------
void lqLidarCorePluginManager::onServerAdded()
{
  this->loadLidarRenderView();
  this->loadLidarPalette();
}

//-----------------------------------------------------------------------------
void lqLidarCorePluginManager::loadLidarRenderView()
{
  pqView* existingView = pqActiveObjects::instance().activeView();

  // If no render view already exist create a new one with the LidarGridView
  if (!existingView)
  {
    pqServer* server = pqActiveObjects::instance().activeServer();
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    if (!server || !builder)
    {
      return;
    }

    pqRenderView* view = qobject_cast<pqRenderView*>(builder->createView("LidarGridView", server));
    if (!view)
    {
      return;
    }
    pqActiveObjects::instance().setActiveView(view);
    builder->addToLayout(view);
  }
}

//-----------------------------------------------------------------------------
void lqLidarCorePluginManager::loadLidarPalette()
{
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  assert(pxm);

  vtkSMProxy* paletteProxy = pxm->GetProxy("settings", "ColorPalette");
  vtkSMProxy* palettePrototype = pxm->GetPrototypeProxy("palettes", "LidarDarkBlueBackground");
  assert(palettePrototype);

  paletteProxy->Copy(palettePrototype);
  paletteProxy->UpdateVTKObjects();
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void lqLidarCorePluginManager::onViewAdded(pqView* view)
{
  pqRenderView* renderView = dynamic_cast<pqRenderView*>(view);
  if (renderView)
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkLidarGridView")
    {
      vtkSMProxy* proxy = renderView->getProxy();
      const double pos[3] = { 0, -72, 18.0 };
      const double focal_point[3] = { 0, 0, 0 };
      const double view_up[3] = { 0, 0.27, 0.96 };
      vtkSMPropertyHelper(proxy, "CameraPosition").Set(pos, 3);
      vtkSMPropertyHelper(proxy, "CameraFocalPoint").Set(focal_point, 3);
      vtkSMPropertyHelper(proxy, "CameraViewUp").Set(view_up, 3);

      vtkSMProxy* lidarGridProxy = vtkSMPropertyHelper(proxy, "LidarGrid").GetAsProxy();
      if (lidarGridProxy)
      {
        pqSettings* settings = pqApplicationCore::instance()->settings();
        bool show = settings->value("MeasurementGridState", true).toBool();
        vtkSMPropertyHelper(lidarGridProxy, "Visibility").Set(show);
        lidarGridProxy->UpdateVTKObjects();
      }

      proxy->UpdateVTKObjects();

      renderView->setCenterOfRotation(0, 0, 0);
      renderView->render();
    }
  }
}
