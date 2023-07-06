/*=========================================================================

  Program: LidarView
  Module:  lqStatusBar.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqStatusBar_h
#define lqStatusBar_h

#include <QLabel>
#include <QStatusBar>

#include "vtkNew.h"

#include "lvApplicationComponentsModule.h"

class pqPipelineSource;

/**
 * lqStatusBar extends QStatusBar to show filename and sensor info
 * of the active source.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqStatusBar : public QStatusBar
{
  Q_OBJECT
  typedef QStatusBar Superclass;

public:
  lqStatusBar(QWidget* parent = nullptr);
  ~lqStatusBar() = default;

protected Q_SLOTS:
  /**
   * Change filename and (sensor info label if present), when the source change.
   */
  void onActiveSourceChanged(pqPipelineSource* activeSource);

private:
  QLabel* filenameLabel;
  QLabel* sensorInfoLabel;

  Q_DISABLE_COPY(lqStatusBar)
};

#endif
