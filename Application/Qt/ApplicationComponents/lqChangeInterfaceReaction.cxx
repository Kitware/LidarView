/*=========================================================================

  Program: LidarView
  Module:  lqChangeInterfaceReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqChangeInterfaceReaction.h"

#include "lqLidarViewManager.h"

//-----------------------------------------------------------------------------
lqChangeInterfaceReaction::lqChangeInterfaceReaction(QAction* parent,
  lqLidarViewManager::InterfaceModes mode)
  : Superclass(parent)
  , interfaceMode(mode)
{
  this->connect(lqLidarViewManager::instance(),
    &lqLidarViewManager::interfaceLayoutUpdated,
    this,
    &lqChangeInterfaceReaction::updateEnableState);
}

//-----------------------------------------------------------------------------
void lqChangeInterfaceReaction::updateEnableState(lqLidarViewManager::InterfaceModes currentMode)
{
  bool currentActionChanged = currentMode == this->interfaceMode;
  this->parentAction()->blockSignals(true);
  this->parentAction()->setChecked(currentActionChanged);
  this->parentAction()->setEnabled(!currentActionChanged);
  this->parentAction()->blockSignals(false);
}

//-----------------------------------------------------------------------------
void lqChangeInterfaceReaction::onTriggered()
{
  lqLidarViewManager::instance()->updateInterfaceLayout(this->interfaceMode);
}
