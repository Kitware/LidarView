/*=========================================================================

  Program: LidarView
  Module:  lqOpenLidarReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqOpenLidarReaction_h
#define lqOpenLidarReaction_h

#include "lqApplicationComponentsModule.h"

#include <pqReaction.h>

#include "vtkSMInterpretersManagerProxy.h"

class pqServer;

/**
 * @ingroup Reactions
 * Reaction that use lqLidarConfigurationDialog to open pcap or streams.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqOpenLidarReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  lqOpenLidarReaction(QAction* parent, vtkSMInterpretersManagerProxy::Mode mode);

  static bool openLidarPcap();
  static bool openLidarPcap(const QString& filename,
    pqServer* server = nullptr,
    vtkSMProxy* defaultProxy = nullptr);
  static bool openLidarStream();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(lqOpenLidarReaction)
  vtkSMInterpretersManagerProxy::Mode InterpreterMode;
};

#endif
