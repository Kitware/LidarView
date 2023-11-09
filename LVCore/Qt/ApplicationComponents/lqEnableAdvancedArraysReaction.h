/*=========================================================================

  Program: LidarView
  Module:  lqEnableAdvancedArraysReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqEnableAdvancedArraysReaction_h
#define lqEnableAdvancedArraysReaction_h

#include <vtkEventQtSlotConnect.h>
#include <vtkSmartPointer.h>

#include <pqReaction.h>

#include "lqApplicationComponentsModule.h"

class pqPipelineSource;
/**
 * @ingroup Reactions
 * Reaction to enable and disable advanced arrays
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqEnableAdvancedArraysReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqEnableAdvancedArraysReaction(QAction* action);

public Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

  /**
   * Monitor the added/deleted source to enable/disable the QAction
   */
  void onSourceAdded(pqPipelineSource* src);
  void onSourceRemoved(pqPipelineSource* src);

  /**
   * Update the button design according to the value of the property
   * "Enable Advanced Arrays" of the first interpreter of the pipeline.
   */
  void updateUI();

private:
  Q_DISABLE_COPY(lqEnableAdvancedArraysReaction)

  void updateIcon(bool setEnable);

  vtkSmartPointer<vtkEventQtSlotConnect> Connection;
};

#endif
