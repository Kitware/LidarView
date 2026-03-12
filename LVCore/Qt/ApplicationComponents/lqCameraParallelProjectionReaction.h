/*=========================================================================

  Program: LidarView
  Module:  lqCameraParallelProjectionReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqCameraParallelProjectionReaction_h
#define lqCameraParallelProjectionReaction_h

#include <pqReaction.h>
#include <vtkSmartPointer.h>

#include "lqApplicationComponentsModule.h"

class pqView;
class pqRenderView;

/**
 * @ingroup Reactions
 * Reaction to change the camera projection type between perspective and parallel
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqCameraParallelProjectionReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqCameraParallelProjectionReaction(QAction* parent);

protected Q_SLOTS:
  /**
   * Called when activeView changed.
   */
  void onViewChanged(pqView* view);

  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

  /**
   * Update the button state according to the camera state.
   */
  void updateUI();

private:
  Q_DISABLE_COPY(lqCameraParallelProjectionReaction)
};

#endif
