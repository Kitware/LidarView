/*=========================================================================

  Program: LidarView
  Module:  lqUpdateConfigurationReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqUpdateConfigurationReaction_h
#define lqUpdateConfigurationReaction_h

#include "lqApplicationComponentsModule.h"

#include <pqReaction.h>

/**
 * @ingroup Reactions
 * Reaction to update a proxy settings using a pqProxyWidgetDialog
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqUpdateConfigurationReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  lqUpdateConfigurationReaction(QAction* parent);

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

  /**
   * Update enabled state
   */
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(lqUpdateConfigurationReaction)
};

#endif
