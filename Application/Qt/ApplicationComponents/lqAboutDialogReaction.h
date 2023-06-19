/*=========================================================================

  Program:   LidarView
  Module:    lqAboutDialogReaction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqAboutDialogReaction_h
#define lqAboutDialogReaction_h

#include "lvApplicationComponentsModule.h"

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * lqAboutDialogReaction used to show the standard about dialog for the
 * application.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqAboutDialogReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqAboutDialogReaction(QAction* parent);

  /**
   * Shows the about dialog for the application.
   */
  static void showAboutDialog();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { lqAboutDialogReaction::showAboutDialog(); }

private:
  Q_DISABLE_COPY(lqAboutDialogReaction)
};

#endif
