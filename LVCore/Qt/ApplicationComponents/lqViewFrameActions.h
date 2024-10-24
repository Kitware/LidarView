/*=========================================================================

  Program: LidarView
  Module:  lqViewFrameActions.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqViewFrameActions_h
#define lqViewFrameActions_h

#include <pqStandardViewFrameActionsImplementation.h>

#include "lqApplicationComponentsModule.h"

class LQAPPLICATIONCOMPONENTS_EXPORT lqViewFrameActions
  : public pqStandardViewFrameActionsImplementation
{
  Q_OBJECT
  Q_INTERFACES(pqViewFrameActionsInterface)

public:
  lqViewFrameActions(QObject* parent = nullptr);
  ~lqViewFrameActions() = default;

  /**
   * This method is called after a frame is assigned to a view. The view may be
   * NULL, indicating the frame has been assigned to an empty view. Frames are
   * never reused (except a frame assigned to an empty view).
   */
  void frameConnected(pqViewFrame* frame, pqView* view) override;

  /**
   * check the XML hints to see if a button with the given name
   * should be added to the view frame
   */
  bool isButtonVisible(const std::string& buttonName, pqView* view) override;
};
#endif // lqViewFrameActions_h
