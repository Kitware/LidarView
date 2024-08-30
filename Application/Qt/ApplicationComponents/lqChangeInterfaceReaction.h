/*=========================================================================

  Program: LidarView
  Module:  lqChangeInterfaceReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqChangeInterfaceReaction_h
#define lqChangeInterfaceReaction_h

#include "lvApplicationComponentsModule.h"

#include <QAction>
#include <QObject>

#include <pqReaction.h>

#include "lqLidarViewManager.h"

/**
 * lqChangeInterfaceReaction is used to switch LidarView different interface mode.
 * Each reaction is bind to a button for one mode and then call lqLidarViewManager
 * to do the switch.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqChangeInterfaceReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  lqChangeInterfaceReaction(QAction* parent, lqLidarViewManager::InterfaceModes mode);

protected Q_SLOTS:
  /**
   * Whenn triggered, call lqLidarViewManager::updateInterfaceLayout()
   */
  void onTriggered() override;

  /**
   * Refresh the enabled state. Applications need not explicitly call this.
   * Refreshed each a new interface is enabled.
   */
  void updateEnableState(lqLidarViewManager::InterfaceModes currentMode);

private:
  Q_DISABLE_COPY(lqChangeInterfaceReaction)

  const lqLidarViewManager::InterfaceModes interfaceMode;
};

#endif
