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

#include <QWidget>

#include <pqActiveObjects.h>
#include <pqDisplayColorWidget.h>
#include <pqEditColorMapReaction.h>
#include <pqResetScalarRangeReaction.h>
#include <pqScalarBarVisibilityReaction.h>
#include <pqSetName.h>

//-----------------------------------------------------------------------------
void lqColorToolbar::constructor()
{
  Ui::lqColorToolbar ui;
  ui.setupUi(this);

  new pqScalarBarVisibilityReaction(ui.actionScalarBarVisibility);
  new pqEditColorMapReaction(ui.actionEditColorMap);
  new pqResetScalarRangeReaction(ui.actionResetRange);
  new pqResetScalarRangeReaction(
    ui.actionRescaleCustomRange, true, pqResetScalarRangeReaction::CUSTOM);

  pqDisplayColorWidget* display_color = new pqDisplayColorWidget(this) << pqSetName("displayColor");
  this->addWidget(display_color);

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)),
    display_color,
    SLOT(setRepresentation(pqDataRepresentation*)));
}
