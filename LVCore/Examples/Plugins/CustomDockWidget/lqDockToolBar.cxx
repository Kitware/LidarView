/*=========================================================================

  Program: LidarView
  Module:  lqDockToolBar.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqDockToolBar.h"

#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QStyle>

#include <pqApplicationCore.h>

//-----------------------------------------------------------------------------
lqDockToolBar::lqDockToolBar(const QString& title, QWidget* parentW)
  : Superclass(title, parentW)
{
  this->constructor();
}

//-----------------------------------------------------------------------------
lqDockToolBar::lqDockToolBar(QWidget* parentW)
  : Superclass(parentW)
{
  this->setWindowTitle(tr("Dock ToolBar"));
  this->constructor();
}

//-----------------------------------------------------------------------------
void lqDockToolBar::constructor()
{
  this->setObjectName("DockToolBar");

  QIcon icon(":/CustomDockResources/Icons/OpenDockWidget.svg");
  QAction* action = this->addAction(icon, tr("Open the dock widget"));
  action->setCheckable(true);

  QDockWidget* dockWidget =
    qobject_cast<QDockWidget*>(pqApplicationCore::instance()->manager("CUSTOM_PANEL"));

  if (dockWidget)
  {
    // Connect action to toggle dock visibility
    this->connect(action,
      &QAction::triggered,
      this,
      [dockWidget](bool checked) { dockWidget->setVisible(checked); });

    // Sync action with dock visibility
    this->connect(dockWidget, &QDockWidget::visibilityChanged, action, &QAction::setChecked);
  }
}
