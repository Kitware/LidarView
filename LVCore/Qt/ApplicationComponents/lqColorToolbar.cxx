/*=========================================================================

  Program: LidarView
  Module:  lqColorToolbar.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqColorToolbar.h"
#include "ui_lqColorToolbar.h"

#include <QToolButton>
#include <QWidget>

#include <pqActiveObjects.h>
#include <pqDisplayColorWidget.h>
#include <pqEditColorMapReaction.h>
#include <pqRescaleScalarRangeReaction.h>
#include <pqScalarBarVisibilityReaction.h>
#include <pqSetName.h>

#include "lqChangeColorPresetReaction.h"

//-----------------------------------------------------------------------------
void lqColorToolbar::constructor()
{
  Ui::lqColorToolbar ui;
  ui.setupUi(this);

  new pqScalarBarVisibilityReaction(ui.actionScalarBarVisibility);
  new lqChangeColorPresetReaction(ui.actionChangeColorPreset);
  new pqEditColorMapReaction(ui.actionEditColorMap);
  new pqRescaleScalarRangeReaction(ui.actionResetRange, true, pqRescaleScalarRangeReaction::DATA);
  new pqRescaleScalarRangeReaction(
    ui.actionRescaleCustomRange, true, pqRescaleScalarRangeReaction::CUSTOM);
  new pqRescaleScalarRangeReaction(
    ui.actionRescaleTemporalRange, true, pqRescaleScalarRangeReaction::TEMPORAL);
  new pqRescaleScalarRangeReaction(
    ui.actionRescaleVisibleRange, true, pqRescaleScalarRangeReaction::VISIBLE);

  pqDisplayColorWidget* display_color = new pqDisplayColorWidget(this) << pqSetName("displayColor");
  // Hide "Components" QComboBox has it is limited used in LidarView
  // A better way to do this would be to add option in pqDisplayColorWidget directly in paraview
  auto widget = display_color->findChild<QWidget*>("Components");
  if (widget)
  {
    widget->setVisible(false);
  }
  this->addWidget(display_color);

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)),
    display_color,
    SLOT(setRepresentation(pqDataRepresentation*)));

  QToolButton* tb = qobject_cast<QToolButton*>(this->widgetForAction(ui.actionChangeColorPreset));
  if (tb)
  {
    tb->setPopupMode(QToolButton::InstantPopup);
  }
}
