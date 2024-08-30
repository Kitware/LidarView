/*=========================================================================

  Program: LidarView
  Module:  lqInterfaceControlsToolbar.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqInterfaceControlsToolbar.h"
#include "ui_lqInterfaceControlsToolbar.h"

#include "lqChangeInterfaceReaction.h"

//-----------------------------------------------------------------------------
void lqInterfaceControlsToolbar::constructor()
{
  Ui::lqInterfaceControlsToolbar ui;
  ui.setupUi(this);

  new lqChangeInterfaceReaction(
    ui.actionLidarViewerSetup, lqLidarViewManager::InterfaceModes::LIDAR_VIEWER);
  new lqChangeInterfaceReaction(
    ui.actionPointCloudToolSetup, lqLidarViewManager::InterfaceModes::POINT_CLOUD_TOOL);
  new lqChangeInterfaceReaction(
    ui.actionAdvancedModeSetup, lqLidarViewManager::InterfaceModes::ADVANCED_MODE);
}
