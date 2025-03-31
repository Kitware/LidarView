/*=========================================================================

  Program:   LidarView
  Module:    lqViewFrameActionsImplementation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqViewFrameActionsImplementation_h
#define lqViewFrameActionsImplementation_h

#include <pqStandardViewFrameActionsImplementation.h>

#include <QList>

#include "lvApplicationComponentsModule.h"

class LVAPPLICATIONCOMPONENTS_EXPORT lqViewFrameActionsImplementation
  : public pqStandardViewFrameActionsImplementation
{
  Q_OBJECT
  typedef pqStandardViewFrameActionsImplementation Superclass;

public:
  lqViewFrameActionsImplementation(QObject* parent = 0);
  ~lqViewFrameActionsImplementation() override = default;

protected:
  /**
   * Returns available view types in LidarView.
   */
  QList<ViewType> availableViewTypes() override;

private:
  Q_DISABLE_COPY(lqViewFrameActionsImplementation);
};

#endif
