/*=========================================================================

  Program: LidarView
  Module:  lqLidarConfigurationDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqLidarConfigurationDialog_h
#define lqLidarConfigurationDialog_h

#include "lqComponentsModule.h"
#include "vtkSMInterpretersManagerProxy.h"

#include <pqDialog.h>

#include <QScopedPointer>

/**
 * lqLidarConfigurationDialog is employed for the creation of a LidarReader or a LidarStream (live
 * source).
 *
 * This dialog utilizes a stack widget of lqInterpreterWidget for each interpreter found by
 * vtkSMInterpretersManagerProxy
 */
class vtkSMProxy;

class LQCOMPONENTS_EXPORT lqLidarConfigurationDialog : public pqDialog
{
  Q_OBJECT
  typedef pqDialog Superclass;

public:
  lqLidarConfigurationDialog(QWidget* Parent,
    vtkSMInterpretersManagerProxy::Mode mode,
    vtkSMProxy* defaultProxy = nullptr);
  ~lqLidarConfigurationDialog() override;

  bool isMultiSensorsEnabled();

  /**
   * Get the vtkSMProxy created for the current selected interpreter.
   * Since this proxy is solely created for populating the Qt widget and populating properties,
   * it is expected to create this proxy using paraview object builder and copy its properties.
   */
  vtkSMProxy* getProxy() const;

  /**
   * Name of QSettings group containing the expressions.
   */
  static constexpr const char* SETTINGS_GROUP() { return "LidarConfigurationDialog"; }

public Q_SLOTS:
  ///@{
  /**
   * Overridden Qt open methods to verify the presence or implementation of at least one
   * interpreter.
   */
  virtual int exec() override;
  virtual void open() override;
  ///@}

private:
  Q_DISABLE_COPY(lqLidarConfigurationDialog)

  class lqInternals;
  QScopedPointer<lqInternals> Internals;
};

#endif // !lqLidarConfigurationDialog_h
