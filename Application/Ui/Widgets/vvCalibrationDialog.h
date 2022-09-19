// Copyright 2013 Velodyne Acoustics, Inc.
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
#ifndef __vvCalibrationDialog_h
#define __vvCalibrationDialog_h

#include <QDialog>
#include <QMatrix4x4>

#include "vvCalibrationStructs.h"

class vtkSMProxy;
class vvCalibrationDialog : public QDialog
{
  Q_OBJECT

public:
  vvCalibrationDialog(QWidget* p = 0, bool isStreamSensor = false);
  vvCalibrationDialog(vtkSMProxy* lidarProxy, vtkSMProxy* GPSProxy, QWidget* p = 0);

  virtual ~vvCalibrationDialog();

  Q_INVOKABLE QString selectedInterpreterName() const;
  Q_INVOKABLE QString selectedCalibrationFile() const;
  Q_INVOKABLE void setCalibrationFile(QString& filename) const;
  Q_INVOKABLE QStringList getAllCalibrationFiles() const;
  Q_INVOKABLE QStringList getCustomCalibrationFiles() const;

  Q_INVOKABLE QMatrix4x4 sensorTransform() const;
  Q_INVOKABLE QMatrix4x4 gpsTransform() const;

  Q_INVOKABLE vvCalibration::TransformConfig getLidarConfig() const;
  Q_INVOKABLE void setLidarConfig(vvCalibration::TransformConfig& conf);

  Q_INVOKABLE vvCalibration::NetworkConfig getLidarNetworkConfig() const;
  Q_INVOKABLE void setLidarNetworkConfig(vvCalibration::NetworkConfig& conf);

  Q_INVOKABLE vvCalibration::TransformConfig getGPSConfig() const;
  Q_INVOKABLE void setGPSConfig(vvCalibration::TransformConfig& conf);

  Q_INVOKABLE vvCalibration::NetworkConfig getGPSNetworkConfig() const;
  Q_INVOKABLE void setGPSNetworkConfig(vvCalibration::NetworkConfig& conf);

  Q_INVOKABLE bool isEnableMultiSensors() const;
  Q_INVOKABLE bool isEnableInterpretGPSPackets() const;

  Q_INVOKABLE bool isShowFirstAndLastFrame() const;

protected:
  void setDefaultConfiguration();

public slots:
  virtual void accept() override;

protected slots:
  void addFile();
  void removeSelectedFile();
  void onCurrentRowChanged(int row);
  void clearAdvancedSettings();

private:
  class pqInternal;
  QScopedPointer<pqInternal> Internal;

  Q_DISABLE_COPY(vvCalibrationDialog)
};

#endif
