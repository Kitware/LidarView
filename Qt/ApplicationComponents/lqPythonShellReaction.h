/*=========================================================================

  Program: LidarView
  Module:  lqPythonShellReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqPythonShellReaction_h
#define lqPythonShellReaction_h

#include "lqApplicationComponentsModule.h"

#include <QAction>
#include <QObject>

#include <pqReaction.h>

/**
 * @ingroup Reactions
 * Reaction for a hide/show the python shell.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqPythonShellReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  lqPythonShellReaction(QAction* parent);
  ~lqPythonShellReaction() override;

  /**
   * Show the python shell
   */
  void togglePythonShellPanel();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { lqPythonShellReaction::togglePythonShellPanel(); }

private:
  Q_DISABLE_COPY(lqPythonShellReaction)
};

#endif
