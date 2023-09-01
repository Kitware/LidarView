/*=========================================================================

  Program: LidarView
  Module:  lqSelectLidarFrameDialog.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqSelectLidarFrameDialog.h"
#include "ui_lqSelectLidarFrameDialog.h"

#include <QDebug>
#include <QMessageBox>

//-----------------------------------------------------------------------------
lqSelectLidarFrameDialog::lqSelectLidarFrameDialog(int nbFrame,
  QWidget* parent,
  FrameMode defaultMode)
  : QDialog(parent)
  , ui(new Ui::lqSelectLidarFrameDialog)
{
  ui->setupUi(this);
  this->ui->StartFrameSpinBox->setMaximum(nbFrame);
  this->ui->StopFrameSpinBox->setMaximum(nbFrame);
  this->ui->StopFrameSpinBox->setValue(nbFrame);

  switch (defaultMode)
  {
    case FrameMode::CURRENT_FRAME:
      this->ui->CurrentFrameRadioButton->setChecked(true);
      break;
    case FrameMode::ALL_FRAMES:
      this->ui->AllFrameRadioButton->setChecked(true);
      break;
    case FrameMode::FRAME_RANGE:
      this->ui->FrameRangeRadioButton->setChecked(true);
      break;
  }
}

//-----------------------------------------------------------------------------
lqSelectLidarFrameDialog::~lqSelectLidarFrameDialog()
{
  delete ui;
}

//-----------------------------------------------------------------------------
lqSelectLidarFrameDialog::FrameMode lqSelectLidarFrameDialog::frameMode() const
{
  if (this->ui->CurrentFrameRadioButton->isChecked())
  {
    return FrameMode::CURRENT_FRAME;
  }
  else if (this->ui->AllFrameRadioButton->isChecked())
  {
    return FrameMode::ALL_FRAMES;
  }
  else if (this->ui->FrameRangeRadioButton->isChecked())
  {
    return FrameMode::FRAME_RANGE;
  }
  qCritical() << "A frame mode should always be selected";
  return FrameMode::CURRENT_FRAME;
}

//-----------------------------------------------------------------------------
int lqSelectLidarFrameDialog::StartFrame() const
{
  FrameMode mode = frameMode();
  if (mode == FrameMode::ALL_FRAMES)
  {
    return 0;
  }
  else if (mode == FrameMode::CURRENT_FRAME)
  {
    return -1;
  }
  else if (mode == FrameMode::FRAME_RANGE)
  {
    return this->ui->StartFrameSpinBox->value();
  }
  qCritical() << "No Frame mode selected!";
  return -1;
}

//-----------------------------------------------------------------------------
int lqSelectLidarFrameDialog::StopFrame() const
{
  FrameMode mode = frameMode();
  if (mode == FrameMode::ALL_FRAMES)
  {
    return this->ui->StopFrameSpinBox->maximum();
  }
  else if (mode == FrameMode::CURRENT_FRAME)
  {
    return -1;
  }
  else if (mode == FrameMode::FRAME_RANGE)
  {
    return this->ui->StopFrameSpinBox->value();
  }
  qCritical() << "No Frame mode selected!";
  return 0;
}

//-----------------------------------------------------------------------------
void lqSelectLidarFrameDialog::accept()
{
  if (this->ui->FrameRangeRadioButton->isChecked() &&
    this->ui->StartFrameSpinBox->value() > this->ui->StopFrameSpinBox->value())
  {
    QMessageBox::critical(this,
      "Invalid frame range",
      "The requested frame range is not valid. "
      "The start frame must be less than or equal to the stop frame.");
    return;
  }
  QDialog::accept();
}
