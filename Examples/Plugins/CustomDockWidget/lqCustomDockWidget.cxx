/*=========================================================================

  Program: LidarView
  Module:  lqCustomDockWidget.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqCustomDockWidget.h"
#include "ui_CustomDockWidget.h"

#include "vtkDummyExternalAPI.h"

#include <QMessageBox>
#include <QString>

#include <pqApplicationCore.h>

//----------------------------------------------------------------------------
lqCustomDockWidget::lqCustomDockWidget(const QString& title, QWidget* parent, Qt::WindowFlags flags)
  : Superclass(title, parent, flags)
{
  this->constructor();
}

//----------------------------------------------------------------------------
lqCustomDockWidget::lqCustomDockWidget(QWidget* parent, Qt::WindowFlags flags)
  : Superclass(parent, flags)
{
  this->constructor();
}

//----------------------------------------------------------------------------
void lqCustomDockWidget::constructor()
{
  this->setWindowTitle("Custom Widget");
  QWidget* thisWidget = new QWidget(this);
  Ui::CustomDockWidget ui;
  ui.setupUi(thisWidget);
  this->setWidget(thisWidget);

  this->connect(
    ui.pressMeButton, &QAbstractButton::clicked, this, &lqCustomDockWidget::onButtonPressed);

  // Register the dock panel to application so it can be connected with toolbar
  pqApplicationCore::instance()->registerManager("CUSTOM_PANEL", this);

  // Hide the widget when plugin is loaded
  this->hide();
}

//----------------------------------------------------------------------------
void lqCustomDockWidget::onButtonPressed()
{
  QString message("Version: ");
  message += vtkDummyExternalAPI::GetVersion().c_str();
  QMessageBox::information(this, "Library version", message);
}
