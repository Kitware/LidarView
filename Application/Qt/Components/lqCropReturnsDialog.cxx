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
#include "lqCropReturnsDialog.h"

#include "ui_lqCropReturnsDialog.h"

#include <vtkSetGet.h> //vtkNotUsed

#include <pqApplicationCore.h>
#include <pqSettings.h>

#include <QDialog>
#include <QPushButton>
#include <QStyle>

#include <sstream>

//-----------------------------------------------------------------------------
class lqCropReturnsDialog::pqInternal : public Ui::lqCropReturnsDialog
{
public:
  pqInternal(QDialog* external)
    : Settings(pqApplicationCore::instance()->settings())
  {
    this->External = external;
    this->setupUi(external);

    this->CancelButton = new QPushButton("Cancel");
    this->CancelButton->setIcon(external->style()->standardIcon(QStyle::SP_DialogCancelButton));

    this->ApplyButton = new QPushButton("Apply");
    this->ApplyButton->setIcon(external->style()->standardIcon(QStyle::SP_DialogOkButton));
    this->ApplyButton->setAutoDefault(true);
    this->ApplyButton->setDefault(true);

    this->ApplyAndSaveButton = new QPushButton("Apply and save for future sessions");
    this->ApplyAndSaveButton->setIcon(external->style()->standardIcon(QStyle::SP_DialogSaveButton));

    this->buttonBox->addButton(this->CancelButton, QDialogButtonBox::ActionRole);
    this->buttonBox->addButton(this->ApplyButton, QDialogButtonBox::ActionRole);
    this->buttonBox->addButton(this->ApplyAndSaveButton, QDialogButtonBox::ActionRole);
  }

  void saveSettings();
  void restoreSettings();
  void SetSphericalSettings();
  void SetCartesianSettings();
  void ActivateSpinBox();
  void DesactivateSpinBox();
  void GetCropRegion(double output[6]);
  void updateRangeValues();

  QDialog* External;

  QPushButton* CancelButton;
  QPushButton* ApplyButton;
  QPushButton* ApplyAndSaveButton;

  double xRange[2];
  double yRange[2];
  double zRange[2];

  pqSettings* const Settings;
};

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::pqInternal::saveSettings()
{
  this->Settings->setValue(
    "LidarPlugin/CropReturnsDialog/EnableCropping", this->CropGroupBox->isChecked());

  this->Settings->setValue(
    "LidarPlugin/CropReturnsDialog/CropOutside", this->CropOutsideCheckBox->isChecked());

  this->Settings->setValue(
    "LidarPlugin/CropReturnsDialog/cartesianRadioButton", this->cartesianRadioButton->isChecked());
  this->Settings->setValue(
    "LidarPlugin/CropReturnsDialog/sphericalRadioButton", this->sphericalRadioButton->isChecked());

  this->Settings->setValue("LidarPlugin/CropReturnsDialog/FirstCornerX", xRange[0]);
  this->Settings->setValue("LidarPlugin/CropReturnsDialog/FirstCornerY", yRange[0]);
  this->Settings->setValue("LidarPlugin/CropReturnsDialog/FirstCornerZ", zRange[0]);
  this->Settings->setValue("LidarPlugin/CropReturnsDialog/SecondCornerX", xRange[1]);
  this->Settings->setValue("LidarPlugin/CropReturnsDialog/SecondCornerY", yRange[1]);
  this->Settings->setValue("LidarPlugin/CropReturnsDialog/SecondCornerZ", zRange[1]);
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::pqInternal::restoreSettings()
{
  this->sphericalRadioButton->setChecked(
    this->Settings->value("LidarPlugin/CropReturnsDialog/sphericalRadioButton", true).toBool());
  this->cartesianRadioButton->setChecked(
    this->Settings->value("LidarPlugin/CropReturnsDialog/cartesianRadioButton", false).toBool());

  if (this->cartesianRadioButton->isChecked())
  {
    this->SetCartesianSettings();
  }

  if (this->sphericalRadioButton->isChecked())
  {
    this->SetSphericalSettings();
  }

  this->CropGroupBox->setChecked(
    this->Settings->value("LidarPlugin/CropReturnsDialog/EnableCropping", false).toBool());

  this->CropOutsideCheckBox->setChecked(
    this->Settings->value("LidarPlugin/CropReturnsDialog/CropOutside", true).toBool());

  this->X1SpinBox->setValue(
    this->Settings->value("LidarPlugin/CropReturnsDialog/FirstCornerX", this->X1SpinBox->value())
      .toDouble());
  this->Y1SpinBox->setValue(
    this->Settings->value("LidarPlugin/CropReturnsDialog/FirstCornerY", this->Y1SpinBox->value())
      .toDouble());
  this->Z1SpinBox->setValue(
    this->Settings->value("LidarPlugin/CropReturnsDialog/FirstCornerZ", this->Z1SpinBox->value())
      .toDouble());
  this->X2SpinBox->setValue(
    this->Settings->value("LidarPlugin/CropReturnsDialog/SecondCornerX", this->X2SpinBox->value())
      .toDouble());
  this->Y2SpinBox->setValue(
    this->Settings->value("LidarPlugin/CropReturnsDialog/SecondCornerY", this->Y2SpinBox->value())
      .toDouble());
  this->Z2SpinBox->setValue(
    this->Settings->value("LidarPlugin/CropReturnsDialog/SecondCornerZ", this->Z2SpinBox->value())
      .toDouble());

  xRange[0] = this->X1SpinBox->value();
  xRange[1] = this->X2SpinBox->value();
  yRange[0] = this->Y1SpinBox->value();
  yRange[1] = this->Y2SpinBox->value();
  zRange[0] = this->Z1SpinBox->value();
  zRange[1] = this->Z2SpinBox->value();

  this->X1SpinBox->setValue(xRange[0]);
  this->X2SpinBox->setValue(xRange[1]);
  this->Y1SpinBox->setValue(yRange[0]);
  this->Y2SpinBox->setValue(yRange[1]);
  this->Z1SpinBox->setValue(zRange[0]);
  this->Z2SpinBox->setValue(zRange[1]);
}

//-----------------------------------------------------------------------------
lqCropReturnsDialog::lqCropReturnsDialog(QWidget* p)
  : QDialog(p)
  , Internal(new pqInternal(this))
{
  connect(
    this->Internal->X1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  connect(
    this->Internal->X2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  connect(
    this->Internal->Y1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  connect(
    this->Internal->Y2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  connect(
    this->Internal->Z1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  connect(
    this->Internal->Z2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  connect(
    this->Internal->cartesianRadioButton, SIGNAL(clicked()), this, SLOT(onCartesianToggled()));
  connect(
    this->Internal->sphericalRadioButton, SIGNAL(clicked()), this, SLOT(onSphericalToggled()));
  connect(this->Internal->CropGroupBox, SIGNAL(clicked()), this, SLOT(onCropGroupBoxToggled()));

  connect(this->Internal->CancelButton, SIGNAL(clicked()), this, SLOT(reject()));
  connect(this->Internal->ApplyButton, SIGNAL(clicked()), this, SLOT(apply()));
  connect(this->Internal->ApplyAndSaveButton, SIGNAL(clicked()), this, SLOT(applyAndSave()));

  this->Internal->restoreSettings();
}

//-----------------------------------------------------------------------------
lqCropReturnsDialog::~lqCropReturnsDialog() {}

//-----------------------------------------------------------------------------
bool lqCropReturnsDialog::croppingEnabled() const
{
  return this->Internal->CropGroupBox->isChecked();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::setCroppingEnabled(bool checked)
{
  this->Internal->CropGroupBox->setChecked(checked);
}

//-----------------------------------------------------------------------------
bool lqCropReturnsDialog::cropOutside() const
{
  return this->Internal->CropOutsideCheckBox->isChecked();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::setCropOutside(bool checked)
{
  this->Internal->CropOutsideCheckBox->setChecked(checked);
}

//-----------------------------------------------------------------------------
QVector3D lqCropReturnsDialog::firstCorner() const
{
  double cropRegion[6];
  this->Internal->GetCropRegion(cropRegion);

  if (this->Internal->sphericalRadioButton->isChecked())
  {
    return QVector3D(cropRegion[0], cropRegion[2], qMin(cropRegion[4], cropRegion[5]));
  }
  else
  {
    return QVector3D(qMin(cropRegion[0], cropRegion[1]),
      qMin(cropRegion[2], cropRegion[3]),
      qMin(cropRegion[4], cropRegion[5]));
  }
}

//-----------------------------------------------------------------------------
QVector3D lqCropReturnsDialog::secondCorner() const
{
  double cropRegion[6];
  this->Internal->GetCropRegion(cropRegion);

  if (this->Internal->sphericalRadioButton->isChecked())
  {
    return QVector3D(cropRegion[1], cropRegion[3], qMax(cropRegion[4], cropRegion[5]));
  }
  else
  {
    return QVector3D(qMax(cropRegion[0], cropRegion[1]),
      qMax(cropRegion[2], cropRegion[3]),
      qMax(cropRegion[4], cropRegion[5]));
  }
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::setFirstCorner(QVector3D corner)
{
  pqInternal* const d = this->Internal.data();
  d->X1SpinBox->setValue(corner.x());
  d->Y1SpinBox->setValue(corner.y());
  d->Z1SpinBox->setValue(corner.z());
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::setSecondCorner(QVector3D corner)
{
  pqInternal* const d = this->Internal.data();
  d->X2SpinBox->setValue(corner.x());
  d->Y2SpinBox->setValue(corner.y());
  d->Z2SpinBox->setValue(corner.z());
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::apply()
{
  QDialog::accept();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::applyAndSave()
{
  this->Internal->saveSettings();
  QDialog::accept();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::onCartesianToggled()
{
  this->Internal->SetCartesianSettings();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::onSphericalToggled()
{
  this->Internal->SetSphericalSettings();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::pqInternal::SetSphericalSettings()
{
  this->ActivateSpinBox();
  // change the labels
  // list of unicode symbol : http://sites.psu.edu/symbolcodes/languages/ancient/greek/greekchart/
  this->XLabel->setText("Rotational angle");
  this->YLabel->setText("Vertical angle");
  this->ZLabel->setText("Distance");

  // Here we take the spherical coordinates used in mathematics (and not physic)
  // (r,theta,phi)
  double minR = 0, maxR = 300;
  double minTheta = -360, maxTheta = 360; // Rotational Angle
  double minPhi = -90, maxPhi = 90;       // Vertical Angle
  // theta is between [minTheta,maxTheta] - Rotational Angle
  this->X1SpinBox->setMinimum(minTheta);
  this->X2SpinBox->setMinimum(minTheta);
  this->X1SpinBox->setMaximum(maxTheta);
  this->X2SpinBox->setMaximum(maxTheta);
  this->X1SpinBox->setValue(0);
  this->X2SpinBox->setValue(360.0);
  // phi is between [minPhi,maxPhi] - Vertical Angle
  this->Y1SpinBox->setMinimum(minPhi);
  this->Y2SpinBox->setMinimum(minPhi);
  this->Y1SpinBox->setMaximum(maxPhi);
  this->Y2SpinBox->setMaximum(maxPhi);
  this->Y1SpinBox->setValue(-90.0);
  this->Y2SpinBox->setValue(90.0);
  // R is positive
  this->Z1SpinBox->setMinimum(minR);
  this->Z2SpinBox->setMinimum(minR);
  this->Z1SpinBox->setMaximum(maxR);
  this->Z2SpinBox->setMaximum(maxR);
  this->Z1SpinBox->setValue(0.0);
  this->Z2SpinBox->setValue(10.0);
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::pqInternal::SetCartesianSettings()
{
  double maxV = 300;
  double minV = -maxV;
  this->ActivateSpinBox();
  // change the labels
  this->XLabel->setText("X");
  this->YLabel->setText("Y");
  this->ZLabel->setText("Z");
  // change the bounds
  // X [-10000,10000]
  this->X1SpinBox->setMinimum(minV);
  this->X2SpinBox->setMinimum(minV);
  this->X1SpinBox->setMaximum(maxV);
  this->X2SpinBox->setMaximum(maxV);
  this->X1SpinBox->setValue(-5.0);
  this->X2SpinBox->setValue(5.0);
  // Y [-10000,10000]
  this->Y1SpinBox->setMinimum(minV);
  this->Y2SpinBox->setMinimum(minV);
  this->Y1SpinBox->setMaximum(maxV);
  this->Y2SpinBox->setMaximum(maxV);
  this->Y1SpinBox->setValue(-5.0);
  this->Y2SpinBox->setValue(5.0);
  // Z [-10000,10000]
  this->Z1SpinBox->setMinimum(minV);
  this->Z2SpinBox->setMinimum(minV);
  this->Z1SpinBox->setMaximum(maxV);
  this->Z2SpinBox->setMaximum(maxV);
  this->Z1SpinBox->setValue(-5.0);
  this->Z2SpinBox->setValue(5.0);
}

//-----------------------------------------------------------------------------
int lqCropReturnsDialog::GetCropMode() const
{
  if (!this->Internal->CropGroupBox->isChecked())
  {
    return 0; // i.e. None
  }
  else if (this->Internal->cartesianRadioButton->isChecked())
  {
    return 1;
  }
  else if (this->Internal->sphericalRadioButton->isChecked())
  {
    return 2;
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::pqInternal::ActivateSpinBox()
{
  this->XLabel->setDisabled(false);
  this->YLabel->setDisabled(false);
  this->ZLabel->setDisabled(false);

  this->X1SpinBox->setDisabled(false);
  this->X2SpinBox->setDisabled(false);

  this->Y1SpinBox->setDisabled(false);
  this->Y2SpinBox->setDisabled(false);

  this->Z1SpinBox->setDisabled(false);
  this->Z2SpinBox->setDisabled(false);
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::pqInternal::GetCropRegion(double output[6])
{
  output[0] = this->X1SpinBox->value();
  output[1] = this->X2SpinBox->value();

  output[2] = this->Y1SpinBox->value();
  output[3] = this->Y2SpinBox->value();

  output[4] = this->Z1SpinBox->value();
  output[5] = this->Z2SpinBox->value();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::pqInternal::DesactivateSpinBox()
{
  this->XLabel->setDisabled(true);
  this->YLabel->setDisabled(true);
  this->ZLabel->setDisabled(true);

  this->X1SpinBox->setDisabled(true);
  this->X2SpinBox->setDisabled(true);

  this->Y1SpinBox->setDisabled(true);
  this->Y2SpinBox->setDisabled(true);

  this->Z1SpinBox->setDisabled(true);
  this->Z2SpinBox->setDisabled(true);
}

void lqCropReturnsDialog::pqInternal::updateRangeValues()
{
  this->xRange[0] = this->X1SpinBox->value();
  this->xRange[1] = this->X2SpinBox->value();
  this->yRange[0] = this->Y1SpinBox->value();
  this->yRange[1] = this->Y2SpinBox->value();
  this->zRange[0] = this->Z1SpinBox->value();
  this->zRange[1] = this->Z2SpinBox->value();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::onSpinBoxChanged(double vtkNotUsed(value))
{
  this->Internal->updateRangeValues();
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::onCropGroupBoxToggled()
{
  this->Internal->X1SpinBox->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->X2SpinBox->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->Y1SpinBox->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->Y2SpinBox->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->Z1SpinBox->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->Z2SpinBox->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->cartesianRadioButton->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->sphericalRadioButton->setDisabled(!this->Internal->CropGroupBox->isChecked());
  this->Internal->CropOutsideCheckBox->setDisabled(!this->Internal->CropGroupBox->isChecked());
}

//-----------------------------------------------------------------------------
void lqCropReturnsDialog::UpdateDialogWithCurrentSetting()
{
  this->onCropGroupBoxToggled();
}
