/*=========================================================================

  Program: LidarView
  Module:  lqColorToolbar.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqColorToolbar_h
#define lqColorToolbar_h

#include <QToolBar>

#include "lqApplicationComponentsModule.h"

/**
 * lqColorToolbar is the toolbar that allows the user to choose the scalar
 * color or solid color for the active representation.
 *
 * @sa pqRepresentationToolbar
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqColorToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  lqColorToolbar(const QString& title, QWidget* parentObject = 0)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  lqColorToolbar(QWidget* parentObject = 0)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private:
  Q_DISABLE_COPY(lqColorToolbar)

  void constructor();
};

#endif
