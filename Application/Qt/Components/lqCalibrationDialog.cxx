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
#include "lqCalibrationDialog.h"

#include "ui_lqCalibrationDialog.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqSettings.h>

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>

#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include "lqHelper.h"

#include <cassert>

//-----------------------------------------------------------------------------
class lqCalibrationDialog::pqInternal : public Ui::lqCalibrationDialog
{
public:
  pqInternal()
    : Settings(pqApplicationCore::instance()->settings())
  {
#if LIDARVIEW_BUILD_VELODYNE
    this->AvailableInterpreters.insert("Velodyne", vvCalibration::Plugin::VELODYNE);
#endif
#if LIDARVIEW_BUILD_HESAI
    this->AvailableInterpreters.insert("Hesai", vvCalibration::Plugin::HESAI);
#endif

    // Velodyne Calibration
    const std::vector<QString> velodyneCalibFiles = { "HDL-32.xml",
      "VLP-16.xml",
      "VLP-32c.xml",
      "Puck Hi-Res.xml",
      "Puck LITE.xml",
      "Alpha Prime.xml" };

    // Hesai Calibration
    const std::vector<QString> hesaiCalibFiles = { "PandarXT32.csv", "Pandar128.csv" };

    QString prefix;
#if defined(_WIN32)
    prefix = QCoreApplication::applicationDirPath() + "/../share/";
#elif defined(__APPLE__)
    prefix = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    prefix = QCoreApplication::applicationDirPath() + "/../share/";
#endif
    for (size_t k = 0; k < velodyneCalibFiles.size(); ++k)
    {
      this->BuiltInCalibrationFiles[vvCalibration::Plugin::VELODYNE]
        << prefix + velodyneCalibFiles[k];
    }
    for (size_t k = 0; k < hesaiCalibFiles.size(); ++k)
    {
      this->BuiltInCalibrationFiles[vvCalibration::Plugin::HESAI] << prefix + hesaiCalibFiles[k];
    }
  }

  void saveInterpreter();
  void saveFileList(const vvCalibration::Plugin& interpreter);
  void saveSelectedRow(const vvCalibration::Plugin& interpreter);

  void restoreInterpreter();
  void restoreSelectedRow(const vvCalibration::Plugin& interpreter);

  void saveSensorTransform();
  void saveGpsTransform();
  void saveLidarPort();
  void saveGpsPort();
  void saveLidarForwardingPort();
  void saveGPSForwardingPort();
  void saveEnableForwarding();
  void saveAdvancedConfiguration();
  void saveForwardIpAddress();
  void saveIsCrashAnalysing();
  void saveEnableMultiSensors();
  void saveEnableInterpretGPSPackets();
  void saveShowFirstAndLastFrame();
  void saveUseRelativeStartTime();

  void restoreSensorTransform();
  void restoreGpsTransform();
  void restoreLidarPort();
  void restoreGpsPort();
  void restoreLidarForwardingPort();
  void restoreGPSForwardingPort();
  void restoreEnableForwarding();
  void restoreAdvancedConfiguration();
  void restoreForwardIpAddress();
  void restoreCrashAnalysing();
  void restoreEnableMultiSensors();
  void restoreEnableInterpretGPSPackets();
  void restoreShowFirstAndLastFrame();
  void restoreUseRelativeStartTime();

  pqSettings* const Settings;
  QMap<QString, vvCalibration::Plugin> AvailableInterpreters;
  QMap<vvCalibration::Plugin, QStringList> BuiltInCalibrationFiles;
};

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveInterpreter()
{
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/Interpreter", this->InterpreterSelectionBox->currentText());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveFileList(const vvCalibration::Plugin& interpreter)
{
  int buildInFileCount = this->BuiltInCalibrationFiles[interpreter].size();

  // Ignore 1 more file for Velodyne because of the HDL-64 live correction
  if (interpreter == vvCalibration::Plugin::VELODYNE)
  {
    buildInFileCount++;
  }

  QStringList files;
  for (int i = buildInFileCount; i < this->CalibrationFileListWidget->count(); ++i)
  {
    files << this->CalibrationFileListWidget->item(i)->data(Qt::UserRole).toString();
  }

  QString path = "LidarPlugin/CalibrationFileDialog/Files" + QString::number((int)interpreter);
  this->Settings->setValue(path, files);
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveSelectedRow(const vvCalibration::Plugin& interpreter)
{
  QString path = "LidarPlugin/CalibrationFileDialog/CurrentRow" + QString::number((int)interpreter);
  this->Settings->setValue(path, this->CalibrationFileListWidget->currentRow());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreInterpreter()
{
  QString interpreter =
    this->Settings->value("LidarPlugin/CalibrationFileDialog/Interpreter").toString();
  QString defaultInterpreter = "Velodyne";
  if (!interpreter.isEmpty())
  {
    this->InterpreterSelectionBox->setCurrentText(interpreter);
  }
  else if (this->AvailableInterpreters.find(defaultInterpreter) !=
    this->AvailableInterpreters.end())
  {
    // Set default interpreter to Velodyne
    this->InterpreterSelectionBox->setCurrentText(defaultInterpreter);
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreSelectedRow(const vvCalibration::Plugin& interpreter)
{
  QString path = "LidarPlugin/CalibrationFileDialog/CurrentRow" + QString::number((int)interpreter);
  int row = this->Settings->value(path).toInt();
  this->CalibrationFileListWidget->setCurrentRow(row);
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveSensorTransform()
{
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarOriginX", this->LidarXSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarOriginY", this->LidarYSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarOriginZ", this->LidarZSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarYaw", this->LidarYawSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarPitch", this->LidarPitchSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarRoll", this->LidarRollSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarTimeOffset", this->lidarTimeOffsetSpinBox->value());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveGpsTransform()
{
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsOriginX", this->GpsXSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsOriginY", this->GpsYSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsOriginZ", this->GpsZSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsYaw", this->GpsYawSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsRoll", this->GpsRollSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsPitch", this->GpsPitchSpinBox->value());
  this->Settings->setValue("LidarPlugin/CalibrationFileDialog/GpsX", this->GpsXSpinBox->value());
  this->Settings->setValue("LidarPlugin/CalibrationFileDialog/GpsY", this->GpsYSpinBox->value());
  this->Settings->setValue("LidarPlugin/CalibrationFileDialog/GpsZ", this->GpsZSpinBox->value());
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsTimeOffset", this->gpsTimeOffsetSpinBox->value());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveLidarPort()
{
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/LidarPort", this->LidarPortSpinBox->value());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveGpsPort()
{
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsPort", this->GPSPortSpinBox->value());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveGPSForwardingPort()
{
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/GpsForwardingPort", this->GPSForwardingPortSpinBox->value());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveLidarForwardingPort()
{
  this->Settings->setValue("LidarPlugin/CalibrationFileDialog/LidarForwardingPort",
    this->LidarForwardingPortSpinBox->value());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveEnableForwarding()
{
  this->Settings->setValue("LidarPlugin/CalibrationFileDialog/EnableForwarding",
    this->EnableForwardingCheckBox->isChecked());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveAdvancedConfiguration()
{
  this->Settings->setValue("LidarPlugin/CalibrationFileDialog/AdvancedConfiguration",
    this->AdvancedConfiguration->isChecked());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveForwardIpAddress()
{
  this->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/ForwardIpAddress", this->ipAddresslineEdit->text());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveIsCrashAnalysing()
{
  // Only save the state if the crash analysing is enabled
  if (this->CrashAnalysisCheckBox->isEnabled())
  {
    this->Settings->setValue("LidarPlugin/CalibrationFileDialog/IsCrashAnalysing",
      this->CrashAnalysisCheckBox->isChecked());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreCrashAnalysing()
{
  // Only restore the state if the crash analysing is enabled
  if (this->CrashAnalysisCheckBox->isEnabled())
  {
    this->CrashAnalysisCheckBox->setChecked(
      this->Settings
        ->value("LidarPlugin/CalibrationFileDialog/IsCrashAnalysing",
          this->CrashAnalysisCheckBox->isChecked())
        .toBool());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveEnableMultiSensors()
{
  // Only save the state if the crash analysing is enabled
  if (this->EnableMultiSensors->isEnabled())
  {
    this->Settings->setValue("LidarPlugin/CalibrationFileDialog/EnableMultiSensors",
      this->EnableMultiSensors->isChecked());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreEnableMultiSensors()
{
  this->EnableMultiSensors->setChecked(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/EnableMultiSensors",
        this->EnableMultiSensors->isChecked())
      .toBool());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveEnableInterpretGPSPackets()
{
  // Only save the state if the Interpreter GPS Packet is enabled
  if (this->EnableInterpretGPSPackets->isEnabled())
  {
    this->Settings->setValue("LidarPlugin/CalibrationFileDialog/EnableInterpretGPSPackets",
      this->EnableInterpretGPSPackets->isChecked());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveShowFirstAndLastFrame()
{
  // Only save the state if the show first and last frame option is enabled
  if (this->ShowFirstAndLastFrame->isEnabled())
  {
    this->Settings->setValue("LidarPlugin/CalibrationFileDialog/ShowFirstAndLastFrame",
      this->ShowFirstAndLastFrame->isChecked());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::saveUseRelativeStartTime()
{
  // Only save the state if the use relative start time option is enabled
  if (this->UseRelativeStartTime->isEnabled())
  {
    this->Settings->setValue("LidarPlugin/CalibrationFileDialog/UseRelativeStartTime",
      this->UseRelativeStartTime->isChecked());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreEnableInterpretGPSPackets()
{
  // Only restore the state if the Interpreter GPS Packet is enabled
  if (this->EnableInterpretGPSPackets->isEnabled())
  {
    this->EnableInterpretGPSPackets->setChecked(
      this->Settings
        ->value("LidarPlugin/CalibrationFileDialog/EnableInterpretGPSPackets",
          this->EnableInterpretGPSPackets->isChecked())
        .toBool());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreSensorTransform()
{
  this->LidarXSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarOriginX", this->LidarXSpinBox->value())
      .toDouble());
  this->LidarYSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarOriginY", this->LidarYSpinBox->value())
      .toDouble());
  this->LidarZSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarOriginZ", this->LidarZSpinBox->value())
      .toDouble());
  this->LidarYawSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarYaw", this->LidarYawSpinBox->value())
      .toDouble());
  this->LidarPitchSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarPitch", this->LidarPitchSpinBox->value())
      .toDouble());
  this->LidarRollSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarRoll", this->LidarRollSpinBox->value())
      .toDouble());

  this->lidarTimeOffsetSpinBox->setValue(
    this->Settings
      ->value(
        "LidarPlugin/CalibrationFileDialog/LidarTimeOffset", this->lidarTimeOffsetSpinBox->value())
      .toDouble());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreGpsTransform()
{
  this->GpsXSpinBox->setValue(
    this->Settings->value(
                    "LidarPlugin/CalibrationFileDialog/GpsOriginX", this->GpsXSpinBox->value())
      .toDouble());
  this->GpsYSpinBox->setValue(
    this->Settings->value(
                    "LidarPlugin/CalibrationFileDialog/GpsOriginY", this->GpsYSpinBox->value())
      .toDouble());
  this->GpsZSpinBox->setValue(
    this->Settings->value(
                    "LidarPlugin/CalibrationFileDialog/GpsOriginZ", this->GpsZSpinBox->value())
      .toDouble());

  this->GpsYawSpinBox->setValue(
    this->Settings->value("LidarPlugin/CalibrationFileDialog/GpsYaw", this->GpsYawSpinBox->value())
      .toDouble());
  this->GpsRollSpinBox->setValue(
    this->Settings->value(
                    "LidarPlugin/CalibrationFileDialog/GpsRoll", this->GpsRollSpinBox->value())
      .toDouble());
  this->GpsPitchSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/GpsPitch", this->GpsPitchSpinBox->value())
      .toDouble());

  this->gpsTimeOffsetSpinBox->setValue(this->Settings
                                         ->value("LidarPlugin/CalibrationFileDialog/GpsTimeOffset",
                                           this->gpsTimeOffsetSpinBox->value())
                                         .toDouble());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreLidarPort()
{
  this->LidarPortSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarPort", this->LidarPortSpinBox->value())
      .toInt());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreGpsPort()
{
  this->GPSPortSpinBox->setValue(
    this->Settings->value(
                    "LidarPlugin/CalibrationFileDialog/GpsPort", this->GPSPortSpinBox->value())
      .toInt());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreGPSForwardingPort()
{
  this->GPSForwardingPortSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/GpsForwardingPort",
        this->GPSForwardingPortSpinBox->value())
      .toInt());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreLidarForwardingPort()
{
  this->LidarForwardingPortSpinBox->setValue(
    this->Settings
      ->value("LidarPlugin/CalibrationFileDialog/LidarForwardingPort",
        this->LidarForwardingPortSpinBox->value())
      .toInt());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreEnableForwarding()
{
  bool tempIsChecked =
    this->Settings->value("LidarPlugin/CalibrationFileDialog/EnableForwarding").toBool();
  this->EnableForwardingCheckBox->setChecked(tempIsChecked);
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreAdvancedConfiguration()
{
  bool tempIsChecked =
    this->Settings->value("LidarPlugin/CalibrationFileDialog/AdvancedConfiguration").toBool();
  this->AdvancedConfiguration->setChecked(tempIsChecked);
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreForwardIpAddress()
{
  this->ipAddresslineEdit->setText(
    this->Settings->value("LidarPlugin/CalibrationFileDialog/ForwardIpAddress").toString());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreShowFirstAndLastFrame()
{
  // Only restore the state if the show first and last frame option is enabled
  if (this->ShowFirstAndLastFrame->isEnabled())
  {
    this->ShowFirstAndLastFrame->setChecked(
      this->Settings
        ->value("LidarPlugin/CalibrationFileDialog/ShowFirstAndLastFrame",
          this->ShowFirstAndLastFrame->isChecked())
        .toBool());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::pqInternal::restoreUseRelativeStartTime()
{
  // Only restore the state if the use relative start time option is enabled
  if (this->UseRelativeStartTime->isEnabled())
  {
    this->UseRelativeStartTime->setChecked(
      this->Settings
        ->value("LidarPlugin/CalibrationFileDialog/UseRelativeStartTime",
          this->UseRelativeStartTime->isChecked())
        .toBool());
  }
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::clearAdvancedSettings()
{
  this->setDefaultConfiguration();
}

namespace
{
QListWidgetItem* createEntry(QString path, bool useBaseName)
{
  QFileInfo info(path);
  QListWidgetItem* wi = new QListWidgetItem();
  if (useBaseName)
  {
    wi->setText(info.baseName());
  }
  else
  {
    wi->setText(info.fileName());
  }
  wi->setToolTip(path);
  wi->setData(Qt::UserRole, path);
  return wi;
}
}

//-----------------------------------------------------------------------------
lqCalibrationDialog::lqCalibrationDialog(QWidget* p, bool isStreamSensor)
  : QDialog(p)
  , Internal(new pqInternal)
{
  this->Internal->setupUi(this);
  this->setDefaultConfiguration();

  // Without settings about the Crash Analysis option on the
  // user's computer, we want the software to not save the log
  // files
  this->Internal->CrashAnalysisCheckBox->setVisible(true);
  this->Internal->CrashAnalysisCheckBox->setEnabled(true);
  this->Internal->CrashAnalysisCheckBox->setChecked(false);

  this->Internal->EnableInterpretGPSPackets->setVisible(true);
  this->Internal->EnableInterpretGPSPackets->setEnabled(true);
  this->Internal->EnableInterpretGPSPackets->setChecked(false);

  this->Internal->ShowFirstAndLastFrame->setEnabled(!isStreamSensor);
  this->Internal->UseRelativeStartTime->setEnabled(!isStreamSensor);

  Q_FOREACH (QString interpreterName, this->Internal->AvailableInterpreters.keys())
  {
    this->Internal->InterpreterSelectionBox->addItem(interpreterName);
  }

  connect(this->Internal->InterpreterSelectionBox,
    SIGNAL(currentTextChanged(const QString&)),
    this,
    SLOT(onCurrentTextChanged(const QString&)));
  connect(this->Internal->CalibrationFileListWidget,
    SIGNAL(currentRowChanged(int)),
    this,
    SLOT(onCurrentRowChanged(int)));
  connect(this->Internal->AddButton, SIGNAL(clicked()), this, SLOT(addFile()));
  connect(this->Internal->RemoveButton, SIGNAL(clicked()), this, SLOT(removeSelectedFile()));
  // The advancedConfiguration checkbox hides the three followings groupbox
  connect(this->Internal->AdvancedConfiguration,
    SIGNAL(toggled(bool)),
    this->Internal->LidarPositionOrientationGroup,
    SLOT(setVisible(bool)));
  // The GPS Position Orientation group (and GPS port) is grey (not settable by the user)
  // if "EnableInterpreterGPSPacket" is disable
  connect(this->Internal->AdvancedConfiguration,
    SIGNAL(toggled(bool)),
    this->Internal->GPSPositionOrientationGroup,
    SLOT(setVisible(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->GpsRollSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->GpsPitchSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->GpsYawSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->GpsXSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->GpsYSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->GpsZSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->GPSPortSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableInterpretGPSPackets,
    SIGNAL(toggled(bool)),
    this->Internal->gpsTimeOffsetSpinBox,
    SLOT(setEnabled(bool)));

  connect(this->Internal->AdvancedConfiguration,
    SIGNAL(toggled(bool)),
    this->Internal->ReaderGroup,
    SLOT(setVisible(bool)));

  connect(this->Internal->AdvancedConfiguration,
    SIGNAL(toggled(bool)),
    this->Internal->NetworkGroup,
    SLOT(setVisible(bool)));
  connect(this->Internal->AdvancedConfiguration,
    SIGNAL(toggled(bool)),
    this->Internal->NetworkForwardingGroup,
    SLOT(setVisible(bool)));
  connect(this->Internal->EnableForwardingCheckBox,
    SIGNAL(toggled(bool)),
    this->Internal->GPSForwardingPortSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableForwardingCheckBox,
    SIGNAL(toggled(bool)),
    this->Internal->LidarForwardingPortSpinBox,
    SLOT(setEnabled(bool)));
  connect(this->Internal->EnableForwardingCheckBox,
    SIGNAL(toggled(bool)),
    this->Internal->ipAddresslineEdit,
    SLOT(setEnabled(bool)));

  connect(this->Internal->ClearSettingsPushButton,
    SIGNAL(clicked()),
    this,
    SLOT(clearAdvancedSettings()));

  this->Internal->restoreInterpreter();
  this->onCurrentTextChanged(this->Internal->InterpreterSelectionBox->currentText());

  this->Internal->restoreSelectedRow(this->selectedInterpreter());
  this->Internal->restoreSensorTransform();
  this->Internal->restoreGpsTransform();
  this->Internal->restoreLidarPort();
  this->Internal->restoreGpsPort();
  this->Internal->restoreEnableForwarding();
  this->Internal->restoreGPSForwardingPort();
  this->Internal->restoreLidarForwardingPort();
  this->Internal->restoreForwardIpAddress();
  this->Internal->restoreCrashAnalysing();
  this->Internal->restoreEnableMultiSensors();
  this->Internal->restoreEnableInterpretGPSPackets();
  this->Internal->restoreEnableForwarding();
  this->Internal->restoreShowFirstAndLastFrame();
  this->Internal->restoreUseRelativeStartTime();
  this->Internal->restoreAdvancedConfiguration();

  const QVariant& geometry =
    this->Internal->Settings->value("LidarPlugin/CalibrationFileDialog/Geometry");
  this->restoreGeometry(geometry.toByteArray());
}

//-----------------------------------------------------------------------------
lqCalibrationDialog::lqCalibrationDialog(vtkSMProxy* lidarProxy, vtkSMProxy* GPSProxy, QWidget* p)
  : lqCalibrationDialog(p)
{
  if (!IsLidarProxy(lidarProxy))
  {
    std::cerr << "lidarProxy is not valid" << std::endl;
    return;
  }

  // Set the Calibration file
  QString lidarCalibrationFile =
    vtkSMPropertyHelper(lidarProxy->GetProperty("CalibrationFileName")).GetAsString();
  // If the calibration is "" that means that the calibration is live
  if (lidarCalibrationFile.isEmpty())
  {
    this->Internal->CalibrationFileListWidget->setCurrentRow(0);
  }
  for (int i = 1; i < this->Internal->CalibrationFileListWidget->count(); ++i)
  {
    QString currentFileName =
      this->Internal->CalibrationFileListWidget->item(i)->data(Qt::UserRole).toString();
    if (lidarCalibrationFile.compare(currentFileName, Qt::CaseInsensitive) == 0)
    {
      this->Internal->CalibrationFileListWidget->setCurrentRow(i);
    }
  }

  if (IsLidarStreamProxy(lidarProxy))
  {
    int lidarPort = vtkSMPropertyHelper(lidarProxy->GetProperty("ListeningPort")).GetAsInt();
    this->Internal->LidarPortSpinBox->setValue(lidarPort);

    bool isForwarding = vtkSMPropertyHelper(lidarProxy->GetProperty("IsForwarding")).GetAsInt();
    this->Internal->EnableForwardingCheckBox->setChecked(isForwarding);

    int lidarForwardedPort =
      vtkSMPropertyHelper(lidarProxy->GetProperty("ForwardedPort")).GetAsInt();
    this->Internal->LidarForwardingPortSpinBox->setValue(lidarForwardedPort);

    std::string forwardedIpAddress =
      vtkSMPropertyHelper(lidarProxy->GetProperty("ForwardedIpAddress")).GetAsString();
    this->Internal->ipAddresslineEdit->setText(QString::fromStdString(forwardedIpAddress));

    // Only restore the state if the crash analysing is enabled
    if (this->Internal->CrashAnalysisCheckBox->isEnabled())
    {
      bool isCrashAnalysing =
        vtkSMPropertyHelper(lidarProxy->GetProperty("IsCrashAnalysing")).GetAsInt();
      this->Internal->CrashAnalysisCheckBox->setChecked(isCrashAnalysing);
    }
  }

  // We can not change the "Enable Multi Sensor" option
  // if we update the calibration of an existing proxy
  this->Internal->EnableMultiSensors->setEnabled(false);

  this->Internal->ShowFirstAndLastFrame->setEnabled(IsLidarReaderProxy(lidarProxy));
  this->Internal->UseRelativeStartTime->setEnabled(IsLidarReaderProxy(lidarProxy));

  std::vector<double> translate;
  std::vector<double> rotate;
  if (GetInterpreterTransform(lidarProxy, translate, rotate))
  {
    this->Internal->LidarXSpinBox->setValue(translate[0]);
    this->Internal->LidarYSpinBox->setValue(translate[1]);
    this->Internal->LidarZSpinBox->setValue(translate[2]);
    this->Internal->LidarRollSpinBox->setValue(rotate[0]);
    this->Internal->LidarPitchSpinBox->setValue(rotate[1]);
    this->Internal->LidarYawSpinBox->setValue(rotate[2]);
  }

  if (GPSProxy && IsPositionOrientationProxy(GPSProxy))
  {
    this->Internal->EnableInterpretGPSPackets->setChecked(true);

    if (IsPositionOrientationStreamProxy(GPSProxy))
    {
      int gpsPort = vtkSMPropertyHelper(GPSProxy->GetProperty("ListeningPort")).GetAsInt();
      this->Internal->GPSPortSpinBox->setValue(gpsPort);

      int gpsForwardingPort =
        vtkSMPropertyHelper(GPSProxy->GetProperty("ForwardedPort")).GetAsInt();
      this->Internal->GPSForwardingPortSpinBox->setValue(gpsForwardingPort);
    }

    std::vector<double> gpsTranslate;
    std::vector<double> gpsRotate;
    GetInterpreterTransform(GPSProxy, gpsTranslate, gpsRotate);
    assert(gpsTranslate.size() == 3);
    assert(gpsRotate.size() == 3);
    this->Internal->GpsXSpinBox->setValue(gpsTranslate[0]);
    this->Internal->GpsYSpinBox->setValue(gpsTranslate[1]);
    this->Internal->GpsZSpinBox->setValue(gpsTranslate[2]);
    this->Internal->GpsRollSpinBox->setValue(gpsRotate[0]);
    this->Internal->GpsPitchSpinBox->setValue(gpsRotate[1]);
    this->Internal->GpsYawSpinBox->setValue(gpsRotate[2]);
  }
  else
  {
    this->Internal->EnableInterpretGPSPackets->setChecked(false);
  }
}

//-----------------------------------------------------------------------------
lqCalibrationDialog::~lqCalibrationDialog()
{
  this->Internal->Settings->setValue(
    "LidarPlugin/CalibrationFileDialog/Geometry", this->saveGeometry());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::setDefaultConfiguration()
{
  const double defaultSensorValue = 0.00;
  const int minAllowedPort = 1024;   // The port between 0 and 1023 are reserved
  const int defaultLidarPort = 2368; // The port between 0 and 1023 are reserved
  const int defaultGpsPort = 8308;   // There is 16 bit to encode the ports : from 0 to 65535
  const QString defaultIpAddress = "127.0.0.1"; // Local host

  // Set the visibility
  this->Internal->LidarPositionOrientationGroup->setVisible(false);
  this->Internal->GPSPositionOrientationGroup->setVisible(false);
  this->Internal->ReaderGroup->setVisible(false);
  this->Internal->NetworkGroup->setVisible(false);
  this->Internal->NetworkForwardingGroup->setVisible(false);

  // set minimum
  this->Internal->LidarPortSpinBox->setMinimum(minAllowedPort);
  this->Internal->GPSPortSpinBox->setMinimum(minAllowedPort);
  this->Internal->GPSForwardingPortSpinBox->setMinimum(minAllowedPort);
  this->Internal->LidarForwardingPortSpinBox->setMinimum(minAllowedPort);
  // set value
  // network configuration values
  this->Internal->LidarPortSpinBox->setValue(defaultLidarPort);
  this->Internal->GPSPortSpinBox->setValue(defaultGpsPort);
  this->Internal->GPSForwardingPortSpinBox->setValue(defaultGpsPort);
  this->Internal->LidarForwardingPortSpinBox->setValue(defaultLidarPort);
  this->Internal->ipAddresslineEdit->setText(defaultIpAddress);
  this->Internal->AdvancedConfiguration->setChecked(false);
  this->Internal->EnableForwardingCheckBox->setChecked(false);
  this->Internal->CrashAnalysisCheckBox->setChecked(false);
  this->Internal->EnableMultiSensors->setChecked(false);
  this->Internal->EnableInterpretGPSPackets->setChecked(false);
  // lidar orientation values
  this->Internal->LidarPitchSpinBox->setValue(defaultSensorValue);
  this->Internal->LidarYawSpinBox->setValue(defaultSensorValue);
  this->Internal->LidarRollSpinBox->setValue(defaultSensorValue);
  // Lidar origin values
  this->Internal->LidarXSpinBox->setValue(defaultSensorValue);
  this->Internal->LidarYSpinBox->setValue(defaultSensorValue);
  this->Internal->LidarZSpinBox->setValue(defaultSensorValue);
  // GPS orientation values
  this->Internal->GpsYawSpinBox->setValue(defaultSensorValue);
  this->Internal->GpsRollSpinBox->setValue(defaultSensorValue);
  this->Internal->GpsPitchSpinBox->setValue(defaultSensorValue);
  // GPS origin values
  this->Internal->GpsXSpinBox->setValue(defaultSensorValue);
  this->Internal->GpsYSpinBox->setValue(defaultSensorValue);
  this->Internal->GpsZSpinBox->setValue(defaultSensorValue);
}

//-----------------------------------------------------------------------------
vvCalibration::Plugin lqCalibrationDialog::selectedInterpreter() const
{
  QString currentInterpreter = this->Internal->InterpreterSelectionBox->currentText();
  QMap<QString, vvCalibration::Plugin>::const_iterator it =
    this->Internal->AvailableInterpreters.find(currentInterpreter);

  if (it != this->Internal->AvailableInterpreters.end())
  {
    return it.value();
  }
  return vvCalibration::Plugin::UNKNOWN;
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::setInterpreter(QString& interpreter)
{
  QMap<QString, vvCalibration::Plugin>::const_iterator it =
    this->Internal->AvailableInterpreters.find(interpreter);
  if (it == this->Internal->AvailableInterpreters.end())
  {
    qCritical() << "Unknown Interpreter Type";
  }
  this->Internal->InterpreterSelectionBox->setCurrentText(interpreter);
}

//-----------------------------------------------------------------------------
QString lqCalibrationDialog::selectedCalibrationFile() const
{
  const int row = this->Internal->CalibrationFileListWidget->currentRow();
  return this->Internal->CalibrationFileListWidget->item(row)->data(Qt::UserRole).toString();
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::setCalibrationFile(QString& filename) const
{
  QStringList existingFiles = this->getAllCalibrationFiles();
  unsigned int idx = 0;
  auto it = std::find(existingFiles.begin(), existingFiles.end(), filename);
  if (it != existingFiles.end())
  {
    idx = it - existingFiles.begin();
  }
  else
  {
    this->Internal->CalibrationFileListWidget->addItem(createEntry(filename, false));
    idx = this->Internal->CalibrationFileListWidget->count() - 1;
    this->Internal->CalibrationFileListWidget->setCurrentRow(idx);
    this->Internal->saveFileList(this->selectedInterpreter());
  }
  this->Internal->CalibrationFileListWidget->setCurrentRow(idx);
}

//-----------------------------------------------------------------------------
QStringList lqCalibrationDialog::getAllCalibrationFiles() const
{
  QStringList calibrationFiles;
  for (int i = 0; i < this->Internal->CalibrationFileListWidget->count(); ++i)
  {
    QString currentFileName =
      this->Internal->CalibrationFileListWidget->item(i)->data(Qt::UserRole).toString();
    calibrationFiles << currentFileName;
  }
  return calibrationFiles;
}

//-----------------------------------------------------------------------------
QStringList lqCalibrationDialog::getCustomCalibrationFiles() const
{
  QString interpreter = QString::number((int)this->selectedInterpreter());
  QString path = "LidarPlugin/CalibrationFileDialog/Files" + interpreter;
  return this->Internal->Settings->value(path).toStringList();
}

//-----------------------------------------------------------------------------
bool lqCalibrationDialog::isEnableMultiSensors() const
{
  return this->Internal->EnableMultiSensors->isChecked();
}

//-----------------------------------------------------------------------------
bool lqCalibrationDialog::isShowFirstAndLastFrame() const
{
  return this->Internal->ShowFirstAndLastFrame->isChecked();
}

//-----------------------------------------------------------------------------
bool lqCalibrationDialog::isUseRelativeStartTime() const
{
  return this->Internal->UseRelativeStartTime->isChecked();
}

//-----------------------------------------------------------------------------
bool lqCalibrationDialog::isEnableInterpretGPSPackets() const
{
  return this->Internal->EnableInterpretGPSPackets->isChecked();
}

//-----------------------------------------------------------------------------
QMatrix4x4 lqCalibrationDialog::sensorTransform() const
{
  // QMatrix4x4 class uses openGL / renderer conventions which
  // is counterintuitive from a linear algebra point of view regarding
  // the sequence of operations (mathematically we first rotate
  // around X, Y, Z and then add T).

  // Rotation computation done according to aerospacial
  // and aeronautics conventions:
  // We have the orientation of referential ref2 according
  // to referential ref1:
  // - Roll is the rotation according to the fixed ref1 X-axis
  // - Pitch is the rotation according to the fixed ref1 Y-axis
  // - Yaw is the rotation according to the fixed ref1 Z-axis
  // - Tx, Ty and Tz are the coordinates of the ref2 position
  // in ref1 referential
  // In this conditions R = Ryaw * Rpitch * Rroll so that
  // Xref1 = R * Xref2

  // Transform the points from Lidar referential
  // to the solid frame referential
  // Perform RX + T
  // QMatrix4x4 has its own convention, the following instructions
  // performed Ryaw*Rpitch*Rroll + T
  QMatrix4x4 transform;
  transform.translate(this->Internal->LidarXSpinBox->value(),
    this->Internal->LidarYSpinBox->value(),
    this->Internal->LidarZSpinBox->value());
  transform.rotate(this->Internal->LidarYawSpinBox->value(), 0.0, 0.0, 1.0);
  transform.rotate(this->Internal->LidarPitchSpinBox->value(), 0.0, 1.0, 0.0);
  transform.rotate(this->Internal->LidarRollSpinBox->value(), 1.0, 0.0, 0.0);

  return transform;
}

//-----------------------------------------------------------------------------
QMatrix4x4 lqCalibrationDialog::gpsTransform() const
{
  // QMatrix4x4 class uses openGL / renderer conventions which
  // is counterintuitive from a linear algebra point of view regarding
  // the sequence of operations (mathematically we first rotate
  // around X, Y, Z and then add T).

  // Rotation computation done according to aerospacial
  // and aeronautics conventions:
  // We have the orientation of referential ref2 according
  // to referential ref1:
  // - Roll is the rotation according to the fixed ref1 X-axis
  // - Pitch is the rotation according to the fixed ref1 Y-axis
  // - Yaw is the rotation according to the fixed ref1 Z-axis
  // - Tx, Ty and Tz are the coordinates of the ref2 position
  // in ref1 referential
  // In this conditions R = Ryaw * Rpitch * Rroll so that
  // Xref1 = R * Xref2

  // Transform the points from Lidar referential
  // to the solid frame referential
  // Perform RX + T
  // QMatrix4x4 has its own convention, the following instructions
  // performed Ryaw*Rpitch*Rroll + T
  QMatrix4x4 transform;
  transform.translate(this->Internal->GpsXSpinBox->value(),
    this->Internal->GpsYSpinBox->value(),
    this->Internal->GpsZSpinBox->value());
  transform.rotate(this->Internal->GpsYawSpinBox->value(), 0.0, 0.0, 1.0);
  transform.rotate(this->Internal->GpsPitchSpinBox->value(), 0.0, 1.0, 0.0);
  transform.rotate(this->Internal->GpsRollSpinBox->value(), 1.0, 0.0, 0.0);

  return transform;
}

//-----------------------------------------------------------------------------
vvCalibration::TransformConfig lqCalibrationDialog::getLidarConfig() const
{
  vvCalibration::TransformConfig config = { this->Internal->LidarYawSpinBox->value(),
    this->Internal->LidarRollSpinBox->value(),
    this->Internal->LidarPitchSpinBox->value(),
    this->Internal->LidarXSpinBox->value(),
    this->Internal->LidarYSpinBox->value(),
    this->Internal->LidarZSpinBox->value(),
    this->Internal->lidarTimeOffsetSpinBox->value() };
  return config;
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::setLidarConfig(vvCalibration::TransformConfig& conf)
{
  this->Internal->LidarYawSpinBox->setValue(conf.yaw);
  this->Internal->LidarRollSpinBox->setValue(conf.roll);
  this->Internal->LidarPitchSpinBox->setValue(conf.pitch);
  this->Internal->LidarXSpinBox->setValue(conf.x);
  this->Internal->LidarYSpinBox->setValue(conf.y);
  this->Internal->LidarZSpinBox->setValue(conf.z);
  this->Internal->lidarTimeOffsetSpinBox->setValue(conf.timeOffset);
}

//-----------------------------------------------------------------------------
vvCalibration::NetworkConfig lqCalibrationDialog::getLidarNetworkConfig() const
{
  vvCalibration::NetworkConfig config = { this->Internal->LidarPortSpinBox->value(),
    this->Internal->LidarForwardingPortSpinBox->value(),
    this->Internal->EnableForwardingCheckBox->isChecked(),
    this->Internal->ipAddresslineEdit->text(),
    this->Internal->CrashAnalysisCheckBox->isChecked() };
  return config;
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::setLidarNetworkConfig(vvCalibration::NetworkConfig& conf)
{
  this->Internal->LidarPortSpinBox->setValue(conf.listenningPort);
  this->Internal->LidarForwardingPortSpinBox->setValue(conf.forwardingPort);
  this->Internal->EnableForwardingCheckBox->setChecked(conf.isForwarding);
  this->Internal->ipAddresslineEdit->setText(conf.ipAddressForwarding);
  this->Internal->CrashAnalysisCheckBox->setChecked(conf.isCrashAnalysing);
}

//-----------------------------------------------------------------------------
vvCalibration::TransformConfig lqCalibrationDialog::getGPSConfig() const
{
  vvCalibration::TransformConfig config = { this->Internal->GpsYawSpinBox->value(),
    this->Internal->GpsRollSpinBox->value(),
    this->Internal->GpsPitchSpinBox->value(),
    this->Internal->GpsXSpinBox->value(),
    this->Internal->GpsYSpinBox->value(),
    this->Internal->GpsZSpinBox->value(),
    this->Internal->gpsTimeOffsetSpinBox->value() };
  return config;
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::setGPSConfig(vvCalibration::TransformConfig& conf)
{
  this->Internal->GpsYawSpinBox->setValue(conf.yaw);
  this->Internal->GpsRollSpinBox->setValue(conf.roll);
  this->Internal->GpsPitchSpinBox->setValue(conf.pitch);
  this->Internal->GpsXSpinBox->setValue(conf.x);
  this->Internal->GpsYSpinBox->setValue(conf.y);
  this->Internal->GpsZSpinBox->setValue(conf.z);
  this->Internal->gpsTimeOffsetSpinBox->setValue(conf.timeOffset);
}

//-----------------------------------------------------------------------------
vvCalibration::NetworkConfig lqCalibrationDialog::getGPSNetworkConfig() const
{
  vvCalibration::NetworkConfig config = { this->Internal->GPSPortSpinBox->value(),
    this->Internal->GPSForwardingPortSpinBox->value(),
    this->Internal->EnableForwardingCheckBox->isChecked(),
    this->Internal->ipAddresslineEdit->text(),
    this->Internal->CrashAnalysisCheckBox->isChecked() };
  return config;
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::setGPSNetworkConfig(vvCalibration::NetworkConfig& conf)
{
  this->Internal->GPSPortSpinBox->setValue(conf.listenningPort);
  this->Internal->GPSForwardingPortSpinBox->setValue(conf.forwardingPort);
  this->Internal->EnableForwardingCheckBox->setChecked(conf.isForwarding);
  this->Internal->ipAddresslineEdit->setText(conf.ipAddressForwarding);
  this->Internal->CrashAnalysisCheckBox->setChecked(conf.isCrashAnalysing);
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::accept()
{
  this->Internal->saveInterpreter();
  this->Internal->saveSensorTransform();
  this->Internal->saveGpsTransform();
  this->Internal->saveLidarPort();
  this->Internal->saveGpsPort();
  this->Internal->saveLidarForwardingPort();
  this->Internal->saveGPSForwardingPort();
  this->Internal->saveEnableForwarding();
  this->Internal->saveAdvancedConfiguration();
  this->Internal->saveForwardIpAddress();
  this->Internal->saveIsCrashAnalysing();
  this->Internal->saveEnableMultiSensors();
  this->Internal->saveEnableInterpretGPSPackets();
  this->Internal->saveShowFirstAndLastFrame();
  this->Internal->saveUseRelativeStartTime();

  QDialog::accept();
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::onCurrentRowChanged(int row)
{
  const vvCalibration::Plugin interpreter = this->selectedInterpreter();
  const int builtInCalibFileSize = this->Internal->BuiltInCalibrationFiles[interpreter].size();
  this->Internal->RemoveButton->setEnabled(row > builtInCalibFileSize);
  // Don't save row when changing interpreter
  if (row != -1)
    this->Internal->saveSelectedRow(interpreter);
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::onCurrentTextChanged(const QString& text)
{
  const vvCalibration::Plugin interpreter = this->Internal->AvailableInterpreters[text];
  this->Internal->CalibrationFileListWidget->clear();

  if (interpreter == vvCalibration::Plugin::VELODYNE)
  {
    QListWidgetItem* liveCalibrationItem = new QListWidgetItem();
    liveCalibrationItem->setText("HDL64 Live Corrections");
    liveCalibrationItem->setToolTip("Get Corrections from the data stream");
    liveCalibrationItem->setData(Qt::UserRole, "");

    this->Internal->CalibrationFileListWidget->addItem(liveCalibrationItem);
  }

  Q_FOREACH (QString fullname, this->Internal->BuiltInCalibrationFiles[interpreter])
  {
    this->Internal->CalibrationFileListWidget->addItem(createEntry(fullname, true));
  }
  Q_FOREACH (QString fullname, this->getCustomCalibrationFiles())
  {
    this->Internal->CalibrationFileListWidget->addItem(createEntry(fullname, false));
  }
  this->Internal->restoreSelectedRow(interpreter);
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::addFile()
{
  QString defaultDir =
    this->Internal->Settings->value("LidarPlugin/OpenData/DefaultDir", QDir::homePath()).toString();

  pqFileDialog dial(pqActiveObjects::instance().activeServer(),
    pqCoreUtilities::mainWidget(),
    tr("Choose Calibration File"),
    defaultDir);
  dial.setObjectName("LidarFileCalibDialog");
  dial.setFileMode(pqFileDialog::ExistingFile);
  if (dial.exec() == QDialog::Rejected)
  {
    return;
  }

  QString fileName = dial.getSelectedFiles().at(0);
  this->Internal->CalibrationFileListWidget->addItem(createEntry(fileName, false));
  this->Internal->CalibrationFileListWidget->setCurrentRow(
    this->Internal->CalibrationFileListWidget->count() - 1);
  this->Internal->saveFileList(this->selectedInterpreter());

  this->Internal->Settings->setValue(
    "LidarPlugin/OpenData/DefaultDir", QFileInfo(fileName).absoluteDir().absolutePath());
}

//-----------------------------------------------------------------------------
void lqCalibrationDialog::removeSelectedFile()
{
  const int row = this->Internal->CalibrationFileListWidget->currentRow();
  if (row >= this->Internal->BuiltInCalibrationFiles.size())
  {
    delete this->Internal->CalibrationFileListWidget->takeItem(row);
    this->Internal->saveFileList(this->selectedInterpreter());
  }
}
