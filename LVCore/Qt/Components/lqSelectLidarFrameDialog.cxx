#include "lqSelectLidarFrameDialog.h"
#include "ui_lqSelectLidarFrameDialog.h"

#include <QMessageBox>
#include <QDebug>

//-----------------------------------------------------------------------------
lqSelectLidarFrameDialog::lqSelectLidarFrameDialog(int nbFrame, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::lqSelectLidarFrameDialog)
{
  ui->setupUi(this);
  this->ui->StartFrameSpinBox->setMaximum(nbFrame);
  this->ui->StopFrameSpinBox->setMaximum(nbFrame);
  this->ui->StopFrameSpinBox->setValue(nbFrame);
}

//-----------------------------------------------------------------------------
lqSelectLidarFrameDialog::~lqSelectLidarFrameDialog()
{
  delete ui;
}

//-----------------------------------------------------------------------------
FrameMode lqSelectLidarFrameDialog::frameMode() const
{
  if (this->ui->CurrentFrameRadioButton->isChecked())
  {
    return FrameMode::CURRENT_FRAME;
  }
  else if(this->ui->AllFrameRadioButton->isChecked())
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
    QMessageBox::critical(this, "Invalid frame range",
      "The requested frame range is not valid. "
      "The start frame must be less than or equal to the stop frame.");
    return;
  }
  QDialog::accept();
}
