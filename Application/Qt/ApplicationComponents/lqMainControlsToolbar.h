/*=========================================================================

  Program: LidarView
  Module:  lqMainControlsToolbar.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqMainControlsToolbar_h
#define lqMainControlsToolbar_h

#include "lvApplicationComponentsModule.h"

#include <QToolBar>

/**
 * lqMainControlsToolbar is the toolbar with actions (and reactions) for the
 * "Main Controls" toolbar in LidarView.
 * Simply instantiate this and put it in your application UI file or
 * QMainWindow to use it.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqMainControlsToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  lqMainControlsToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  lqMainControlsToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private:
  Q_DISABLE_COPY(lqMainControlsToolbar)

  void constructor();
};

#endif
