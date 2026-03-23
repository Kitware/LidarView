/*=========================================================================

  Program:   LidarView
  Module:    lqLidarCorePluginManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   lqLidarCorePluginManager
 *
 * This class is a autostart class for the plugin LidarCorePlugin.
 * Watch few paraview signals to improve interraction between LidarView and this plugin.
 */

#ifndef lqLidarCorePluginManager_h
#define lqLidarCorePluginManager_h

#include <QObject>

class pqView;

class lqLidarCorePluginManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  lqLidarCorePluginManager(QObject* p = 0);
  ~lqLidarCorePluginManager();

  ///@{
  /**
   * Called at plugin load / unload time.
   */
  void onStartup(){};
  void onShutdown(){};
  ///@}

private Q_SLOTS:

  /**
   * Slot triggered each time a plugin is updated.
   * Calls loadLidarRenderView() and loadLidarPalette()
   */
  void onPluginLoaded();

  /**
   * Slot triggered each time a server is added/reset.
   */
  void onServerAdded();

  /**
   * Slot triggered each time a view is added.
   * Used to watch when a LidarGridView is added, it will change camera
   * position.
   */
  void onViewAdded(pqView* view);

private:
  /**
   * Used to load the LidarGridView once added to the proxy list,
   * only add the RenderView if no other RenderView is not already active.
   */
  void loadLidarRenderView();

  /**
   * Used to load the default color palette.
   */
  void loadLidarPalette();

private:
  Q_DISABLE_COPY(lqLidarCorePluginManager)
};

#endif
