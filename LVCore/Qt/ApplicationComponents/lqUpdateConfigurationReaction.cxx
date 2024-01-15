/*=========================================================================

  Program: LidarView
  Module:  lqUpdateConfigurationReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqUpdateConfigurationReaction.h"

#include <vtkSMProxy.h>

#include <pqActiveObjects.h>
#include <pqPipelineSource.h>
#include <pqProxyWidgetDialog.h>

//-----------------------------------------------------------------------------
lqUpdateConfigurationReaction::lqUpdateConfigurationReaction(QAction* parent)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
void lqUpdateConfigurationReaction::onTriggered()
{
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  vtkSMProxy* proxy = source->getProxy();
  pqProxyWidgetDialog dialog(proxy);
  dialog.setWindowTitle(tr("Change Configuration"));
  dialog.setObjectName("UpdateConfigurationDialog");
  dialog.exec();
}
