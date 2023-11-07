/*=========================================================================

  Program: LidarView
  Module:  lqInterfaceControlsToolbar.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqInterfaceControlsToolbar_h
#define lqInterfaceControlsToolbar_h

#include "lvApplicationComponentsModule.h"

#include <QToolBar>

/**
 * lqInterfaceControlsToolbar is the toolbar used to switch LidarView interface.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqInterfaceControlsToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  lqInterfaceControlsToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  lqInterfaceControlsToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private:
  Q_DISABLE_COPY(lqInterfaceControlsToolbar)

  void constructor();
};

#endif
