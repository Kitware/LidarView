/*=========================================================================

  Program: LidarView
  Module:  lqSavePCAPReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef LQSAVEPCAPREACTION_H
#define LQSAVEPCAPREACTION_H

#include "lqSaveLidarFrameReaction.h"

#include <vtkSmartPointer.h>

#include "lqApplicationComponentsModule.h"

class pqPipelineSource;
class vtkSMSourceProxy;
class vtkSMProxy;

/**
 * @ingroup Reactions
 * Save a pcap file. Can be used to crop a pcap
 *
 * The reaction provide some basic UI to let the user select the frame to save:
 * - current frame
 * - all frame
 * - frame interval
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqSavePcapReaction : public lqSaveLidarFrameReaction
{
  Q_OBJECT
  typedef lqSaveLidarFrameReaction Superclass;

public:
  lqSavePcapReaction(QAction* action, bool displaySettings = false);

  /**
   *  Save the frame generate by the lidar "lidar"
   *  Start and Stop correspond to the timestep index. Both are included [start; stop]
   *  In case both are -1, the current timestep is saved
   */
  virtual bool saveFrame(vtkSMProxy* lidar, int start = -1, int stop = -1) override;

public Q_SLOTS:
  /**
   * Updates the enabled state.
   */
  void updateEnableState() override;

  /**
   * Called when the action is triggered.
   */
  virtual void onTriggered() override;

private:
  Q_DISABLE_COPY(lqSavePcapReaction)
};

#endif // LQSAVEPCAPREACTION_H
