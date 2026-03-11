/*=========================================================================

  Program: LidarView
  Module:  lqRulersToolbar.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqRulersToolbar.h"
#include "ui_lqRulersToolbar.h"

#include <QAction>

#include "lqCameraParallelProjectionReaction.h"
#include "lqRulerReaction.h"

#include <functional>

//-----------------------------------------------------------------------------
void lqRulersToolbar::constructor()
{
  Ui::lqRulersToolbar ui;
  ui.setupUi(this);

  auto ruler3D =
    new lqRulerReaction(ui.actionMeasurePointToPoint, lqRulerReaction::Mode::BETWEEN_3D_POINTS);
  auto ruler2D =
    new lqRulerReaction(ui.actionMeasureInParallelPlane, lqRulerReaction::Mode::BETWEEN_2D_POINTS);
  new lqCameraParallelProjectionReaction(ui.actionToggleProjection);

  auto handleActionToggled = [](bool checked, lqRulerReaction* rulerReaction)
  {
    if (checked)
    {
      rulerReaction->onDisable();
    }
  };

  QObject::connect(ui.actionMeasurePointToPoint,
    &QAction::toggled,
    this,
    std::bind(handleActionToggled, std::placeholders::_1, ruler2D));
  QObject::connect(ui.actionMeasureInParallelPlane,
    &QAction::toggled,
    this,
    std::bind(handleActionToggled, std::placeholders::_1, ruler3D));
}
