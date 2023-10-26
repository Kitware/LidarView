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

class LVAPPLICATIONCOMPONENTS_EXPORT lqLidarViewManager : public lqLidarCoreManager
{

  Q_OBJECT
  typedef lqLidarCoreManager Superclass;

public:
  lqLidarViewManager(QObject* parent = nullptr);
  ~lqLidarViewManager() override = default;

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
};

#endif
