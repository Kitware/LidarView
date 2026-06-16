/*=========================================================================

  Program: LidarView
  Module:  lqRulersToolbar.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqRulersToolbar_h
#define lqRulersToolbar_h

#include "lqApplicationComponentsModule.h"

#include <QToolBar>

/**
 * lqRulersToolbar
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqRulersToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  lqRulersToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  lqRulersToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private:
  Q_DISABLE_COPY(lqRulersToolbar)
  void constructor();
};

#endif // lqRulersToolbar_h
