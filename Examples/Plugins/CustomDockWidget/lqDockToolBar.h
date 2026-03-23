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

#include <QToolBar>

class lqDockToolBar : public QToolBar
{
  Q_OBJECT;
  using Superclass = QToolBar;

public:
  lqDockToolBar(const QString& title, QWidget* parent = nullptr);
  lqDockToolBar(QWidget* parent = nullptr);
  ~lqDockToolBar() override = default;

private:
  Q_DISABLE_COPY(lqDockToolBar);
  void constructor();
};
