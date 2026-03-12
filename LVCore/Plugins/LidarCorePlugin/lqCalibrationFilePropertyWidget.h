/*=========================================================================

  Program:   LidarView
  Module:    lqCalibrationFilePropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqCalibrationFilePropertyWidget_h
#define lqCalibrationFilePropertyWidget_h

#include "pqPropertyWidget.h"

#include <QScopedPointer>

/**
 * lqCalibrationFilePropertyWidget bears a resemblance to ParaView's pqFileListPropertyWidget in
 * many aspects, but it differs in several key points:
 *  - Includes a checkbox for toggling the visibility of the QListWidget
 *  - It automatically caches added files.
 *  - It displays only the filename with extension, not the full path.
 *
 * This widget can be used in paraview xml by adding `panel_widget="calibration_file_widget"`
 * decorator in a <StringVectorProperty />
 */
class lqCalibrationFilePropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

  Q_PROPERTY(
    QString filenameProp READ currentFilename WRITE setCurrentFilename NOTIFY filenameChanged);

public:
  lqCalibrationFilePropertyWidget(vtkSMProxy* smproxy,
    vtkSMProperty* smproperty,
    QWidget* parentObject = nullptr);
  virtual ~lqCalibrationFilePropertyWidget();

Q_SIGNALS:
  void filenameChanged();

private:
  Q_DISABLE_COPY(lqCalibrationFilePropertyWidget)

  QString currentFilename() const;
  void setCurrentFilename(QString);

  class lqInternals;
  QScopedPointer<lqInternals> Internals;
};

#endif
