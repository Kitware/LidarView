// Copyright 2014 Velodyne Acoustics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef lqCropReturnsDialog_h
#define lqCropReturnsDialog_h

#include "lvComponentsModule.h"

#include <QDialog>
#include <QVector3D>

class LVCOMPONENTS_EXPORT lqCropReturnsDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(bool croppingEnabled READ croppingEnabled WRITE setCroppingEnabled)
  Q_PROPERTY(bool cropOutside READ cropOutside WRITE setCropOutside)
  Q_PROPERTY(QVector3D firstCorner READ firstCorner WRITE setFirstCorner)
  Q_PROPERTY(QVector3D secondCorner READ secondCorner WRITE setSecondCorner)

public:
  lqCropReturnsDialog(QWidget* p = 0);
  virtual ~lqCropReturnsDialog();

  bool croppingEnabled() const;
  bool cropOutside() const;

  void setCroppingEnabled(bool);
  void setCropOutside(bool);

  QVector3D firstCorner() const;
  QVector3D secondCorner() const;

  void setFirstCorner(QVector3D);
  void setSecondCorner(QVector3D);

  Q_INVOKABLE void UpdateDialogWithCurrentSetting();

  Q_INVOKABLE int GetCropMode() const;

public Q_SLOTS:
  virtual void apply();
  virtual void applyAndSave();
  void onCartesianToggled();
  void onSphericalToggled();
  void onCropGroupBoxToggled();
  void onSpinBoxChanged(double value);

private:
  class pqInternal;
  QScopedPointer<pqInternal> Internal;

  Q_DISABLE_COPY(lqCropReturnsDialog)
};

#endif
