/*=========================================================================

  Program: LidarView
  Module:  lq2DBoxPropertyWidget.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lq2DBoxPropertyWidget.h"

#include <vtkSMNewWidgetRepresentationProxy.h>
#include <vtkSMPropertyGroup.h>
#include <vtkSMProxy.h>
#include <vtkSMUncheckedPropertyHelper.h>

#include <QCheckBox>

//-----------------------------------------------------------------------------
lq2DBoxPropertyWidget::lq2DBoxPropertyWidget(vtkSMProxy* smproxy,
  vtkSMPropertyGroup* smgroup,
  QWidget* parentObject)
  : Superclass(smproxy, smgroup, parentObject, false)
{
  QCheckBox* enableRotation = this->findChild<QCheckBox*>("enableRotation");
  // Deactivate rotation if found
  if (enableRotation)
  {
    enableRotation->toggle();
    enableRotation->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
lq2DBoxPropertyWidget::~lq2DBoxPropertyWidget() = default;
