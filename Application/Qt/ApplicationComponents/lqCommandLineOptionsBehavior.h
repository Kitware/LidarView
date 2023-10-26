/*=========================================================================

  Program: LidarView
  Module:  lqCommandLineOptionsBehavior.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   lqCommandLineOptionsBehavior
 *
 * This class use pqCommandLineOptionsBehavior methods internally, it is needed
 * to be able to test LidarView with the LidarGridView.
 */

#ifndef lqCommandLineOptionsBehavior_h
#define lqCommandLineOptionsBehavior_h

#include <QObject>

#include "lvApplicationComponentsModule.h"

class pqView;

class LVAPPLICATIONCOMPONENTS_EXPORT lqCommandLineOptionsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  lqCommandLineOptionsBehavior(QObject* parent = nullptr);

Q_SIGNALS:
  /**
   * Signal that trigger processCommandLineOptions() when
   * the render view is ready to be tested.
   */
  void lidarGridViewActive();

private Q_SLOTS:
  /**
   * Slot triggered each time the active view is changed.
   * Check if the active view is a LidarGridView and emit only one
   * lidarGridViewActive.
   */
  void onActiveViewChanged(pqView* view);

  /**
   * Called by a timer to emit lidarGridViewActive
   */
  void emitLidarGridViewActive();

  /**
   * Copy of pqCommandLineOptionsBehavior::processCommandLineOptions()
   */
  void processCommandLineOptions();

private:
  Q_DISABLE_COPY(lqCommandLineOptionsBehavior)

  bool signalEmitted = false;
};

#endif
