/*=========================================================================

  Program:   LidarView
  Module:    lqWelcomeDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqWelcomeDialog_h
#define lqWelcomeDialog_h

#include <QDialog>
#include <QLabel>

#include "lqLidarViewManager.h"

#include <array>

// For export module
#include "lvApplicationComponentsModule.h"

namespace Ui
{
class lqWelcomeDialog;
}

/**
 * lqWelcomeDialog is the dialog shown at LidarView first launch.
 * Helps to choose wich interface mode they want to start with.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqWelcomeDialog : public QDialog
{
  Q_OBJECT

public:
  lqWelcomeDialog(QWidget* Parent);
  ~lqWelcomeDialog() override;

private Q_SLOTS:
  ///@{
  /**
   * React to checkbox events
   */
  void currentModeChanged(int modeIdx);
  void onDoNotShowAgainStateChanged(int state);
  ///@}

private:
  Q_DISABLE_COPY(lqWelcomeDialog)
  Ui::lqWelcomeDialog* const Ui;

  lqLidarViewManager::InterfaceModes currentMode = lqLidarViewManager::LIDAR_VIEWER;
  std::array<QLabel*, 3> modeLabelList;
};

#endif // !lqWelcomeDialog_h
