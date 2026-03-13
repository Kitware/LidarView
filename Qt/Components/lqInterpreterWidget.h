/*=========================================================================

  Program: LidarView
  Module:  lqInterpreterWidget.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqInterpreterWidgetBase_h
#define lqInterpreterWidgetBase_h

#include <vector>

#include <QScopedPointer>
#include <QWidget>

#include "lqComponentsModule.h"

class vtkSMProxy;
class pqProxyWidget;

/**
 * lqInterpreterWidget generate a pqProxyWidget for lidar reader / stream proxies.
 * If a lidar pose proxies is specified, a checkbox will be displayed to switch between the two.
 */
class LQCOMPONENTS_EXPORT lqInterpreterWidget : public QWidget
{
  Q_OBJECT;
  typedef QWidget Superclass;

public:
  lqInterpreterWidget(QWidget* parentW, const char* proxyName);
  lqInterpreterWidget(QWidget* parentW, const char* lidarProxyName, const char* poseProxyName);
  virtual ~lqInterpreterWidget();

  /**
   * Create a pqProxyWidget using a specified proxyName.
   * This will additionally hide the FileName property and restore proxy custom settings.
   */
  static pqProxyWidget* createProxyWidget(QWidget* parent, const char* proxyName);

  /**
   * Get the vtkSMProxy created under the pqProxyWidget.
   */
  vtkSMProxy* getProxy();

  /**
   * Attempt to transfer the provided proxy values to the widget underlying proxies,
   * ensuring compatibility with the proxy name and group.
   * If successful, return true.
   */
  bool trySetProxySettings(vtkSMProxy* targetProxy);

public Q_SLOTS:
  ///@{
  /**
   * Slots functions used to activate advanced mode, save, or restore settings.
   */
  void enableAdvancedMode(bool enabled);
  void saveAsDefaults();
  void restoreDefaults();
  ///@}

private Q_SLOTS:
  void changeGNSSState(int state);

private:
  Q_DISABLE_COPY(lqInterpreterWidget)

  class lqInternals;
  QScopedPointer<lqInternals> Internals;
};

#endif
