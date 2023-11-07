/*=========================================================================

  Program: LidarView
  Module:  lqMeasurementGridReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqMeasurementGridReaction_h
#define lqMeasurementGridReaction_h

#include "lqApplicationComponentsModule.h"

#include <QAction>
#include <QObject>

#include <pqReaction.h>

/**
 * @ingroup Reactions
 * Reaction for a show measurement grid button.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqMeasurementGridReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  lqMeasurementGridReaction(QAction* parent);

public Q_SLOTS:
  /**
   * Refresh the enabled state. Applications need not explicitly call this.
   * The button will be only available if the current view is a vtkLidarGridView.
   */
  void onRefreshButton();

  /**
   * Update the grid visibility, if the current active view is a vtkLidarGridView.
   */
  static void updateGridVisibility(bool show);

private:
  Q_DISABLE_COPY(lqMeasurementGridReaction)
};

#endif
