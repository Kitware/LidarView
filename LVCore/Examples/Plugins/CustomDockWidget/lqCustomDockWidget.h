/*=========================================================================

  Program: LidarView
  Module:  lqCustomDockWidget.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <QDockWidget>

class lqCustomDockWidget : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  lqCustomDockWidget(const QString& title,
    QWidget* parent = nullptr,
    Qt::WindowFlags flags = Qt::WindowFlags());
  lqCustomDockWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private:
  void constructor();

private Q_SLOTS:
  void onButtonPressed();
};
