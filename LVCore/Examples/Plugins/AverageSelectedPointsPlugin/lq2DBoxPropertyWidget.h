/*=========================================================================

  Program: LidarView
  Module:  lq2DBoxPropertyWidget.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lq2DBoxPropertyWidget_h
#define lq2DBoxPropertyWidget_h

#include "pqBoxPropertyWidget.h"

/**
 * Example that have the same features than pqBoxPropertyWidget
 * while deactivating rotations.
 */
class lq2DBoxPropertyWidget : public pqBoxPropertyWidget
{
  Q_OBJECT
  typedef pqBoxPropertyWidget Superclass;

public:
  lq2DBoxPropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~lq2DBoxPropertyWidget() override;

private:
  Q_DISABLE_COPY(lq2DBoxPropertyWidget)
};

#endif
