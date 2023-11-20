/*=========================================================================

  Program: LidarView
  Module:  LidarViewMainWindow.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef LidarViewMainWindow_h
#define LidarViewMainWindow_h

#include <QMainWindow>

class LidarViewMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  LidarViewMainWindow();
  virtual ~LidarViewMainWindow() override;

protected:
  void dragEnterEvent(QDragEnterEvent* evt) override;
  void dropEvent(QDropEvent* evt) override;
  void showEvent(QShowEvent* evt) override;
  void closeEvent(QCloseEvent* evt) override;

protected Q_SLOTS:
  void handleMessage(const QString&, int);
  void showWelcomeDialog();
  void updateFontSize();

private:
  Q_DISABLE_COPY(LidarViewMainWindow)

  class lqInternals;
  lqInternals* Internals;
};

#endif
