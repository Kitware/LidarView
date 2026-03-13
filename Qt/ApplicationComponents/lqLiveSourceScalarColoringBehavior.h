/*=========================================================================

  Program: LidarView
  Module:  lqLiveSourceScalarColoringBehavior.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqLiveSourceScalarColoringBehavior_h
#define lqLiveSourceScalarColoringBehavior_h

#include <QObject>

#include "lqApplicationComponentsModule.h"

class pqPipelineSource;
class vtkSMSourceProxy;

/**
 * @class lqLiveSourceScalarColoringBehavior
 * @ingroup Behaviors
 *
 * lqLiveSourceScalarColoringBehavior helps to set ScalarColoring,
 * once a LiveSource has received data with an active scalar for the first time.
 * Without it the representation don't set color informations properly
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqLiveSourceScalarColoringBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  lqLiveSourceScalarColoringBehavior(QObject* parent = 0);

protected Q_SLOTS:
  /**
   * @brief sourceAdded check if the source is a live source and connect
   */
  void onSourceAdded(pqPipelineSource* src);

  virtual void onDataUpdated(pqPipelineSource* src);

protected:
  /**
   * @brief Try to set and rescale the scalar coloring for each representation
   * Return true, if procedure has completed and false it the procedure should
   * be tried again later.
   */
  virtual bool TrySetScalarColoring(pqPipelineSource* src);

private:
  Q_DISABLE_COPY(lqLiveSourceScalarColoringBehavior)
};

#endif // lqLiveSourceScalarColoringBehavior_h
