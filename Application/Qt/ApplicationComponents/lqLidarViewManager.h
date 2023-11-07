/*=========================================================================

  Program:   LidarView
  Module:    lqLidarViewManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqLidarViewManager_h
#define lqLidarViewManager_h

#include <lqLidarCoreManager.h>

#include "lvApplicationComponentsModule.h"

#include <QScopedPointer>

class LVAPPLICATIONCOMPONENTS_EXPORT lqLidarViewManager : public lqLidarCoreManager
{

  Q_OBJECT
  typedef lqLidarCoreManager Superclass;

public:
  enum InterfaceModes
  {
    LIDAR_VIEWER = 0,
    POINT_CLOUD_TOOL,
    ADVANCED_MODE
  };

  lqLidarViewManager(QObject* parent = nullptr);
  ~lqLidarViewManager() override;

  /**
   * Returns the pqPVApplicationCore instance. If no pqPVApplicationCore has been
   * created then return nullptr.
   */
  static lqLidarViewManager* instance()
  {
    return qobject_cast<lqLidarViewManager*>(Superclass::instance());
  }

  /**
   * Change ParaView default settings value such as background color
   * and LUT for lidar scalars.
   */
  static void SetLidarViewDefaultSettings();

  /**
   * Switch to a new interface layout defined by InterfaceModes.
   * The modes are defined in json config files. Mode layout configurations
   * are stored in memory, which means that when you switch modes while
   * LidarView is running, any changes to the layout will be preserved.
   *
   * Note that a signal interfaceLayoutUpdated will be emitted with the
   * new mode, and it will be saved in GeneralSettings.InterfaceMode
   */
  void updateInterfaceLayout(InterfaceModes mode);

  /**
   * Restore last interface layout used.
   */
  void restoreSavedInterfaceLayout();

Q_SIGNALS:
  /**
   * Signal emitted when a new interface mode is changed.
   */
  void interfaceLayoutUpdated(InterfaceModes mode);

private:
  Q_DISABLE_COPY(lqLidarViewManager)

  struct lqInternals;
  QScopedPointer<lqInternals> Internals;
};

#endif
