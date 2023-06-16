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
#include "lqSelectFramesDialog.h"

#include "ui_lqSelectFramesDialog.h"

#include <pqApplicationCore.h>
#include <pqSettings.h>

#include <QMessageBox>

//-----------------------------------------------------------------------------
class lqSelectFramesDialog::pqInternal : public Ui::lqSelectFramesDialog
{
};

//-----------------------------------------------------------------------------
lqSelectFramesDialog::lqSelectFramesDialog(QWidget* p)
  : QDialog(p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  this->Internal->FrameStart->clearFocus();
}

//-----------------------------------------------------------------------------
lqSelectFramesDialog::~lqSelectFramesDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::accept()
{
  if (this->Internal->FrameStop->value() < this->Internal->FrameStart->value())
  {
    QMessageBox::critical(this,
      "Invalid frame range",
      "The requested frame range is not valid. "
      "The start frame must be less than or equal to the stop frame.");
    return;
  }

  this->saveState();
  QDialog::accept();
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::frameMode() const
{
  if (this->Internal->CurrentFrameButton->isChecked())
  {
    return CURRENT_FRAME;
  }
  else if (this->Internal->AllFramesButton->isChecked())
  {
    return ALL_FRAMES;
  }
  else
  {
    return FRAME_RANGE;
  }
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameMode(int frameMode)
{
  if (frameMode == CURRENT_FRAME)
  {
    this->Internal->CurrentFrameButton->setChecked(true);
  }
  else if (frameMode == ALL_FRAMES)
  {
    this->Internal->AllFramesButton->setChecked(true);
  }
  else if (frameMode == FRAME_RANGE)
  {
    this->Internal->FrameRangeButton->setChecked(true);
  }
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::frameStart() const
{
  return this->Internal->FrameStart->value();
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::frameStop() const
{
  return this->Internal->FrameStop->value();
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::frameStride() const
{
  return this->Internal->FrameStride->value();
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameStart(int frameStart)
{
  this->Internal->FrameStart->setValue(frameStart);
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameStop(int frameStop)
{
  this->Internal->FrameStop->setValue(frameStop);
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameStride(int frameStride)
{
  this->Internal->FrameStride->setValue(frameStride);
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::framePack() const
{
  if (this->Internal->FilePerFrameButton->isChecked())
  {
    return FILE_PER_FRAME;
  }
  else
  {
    return SINGLE_FILE;
  }
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFramePack(int framePack)
{
  if (framePack == SINGLE_FILE)
  {
    this->Internal->SingleFileButton->setChecked(true);
  }
  else if (framePack == FILE_PER_FRAME)
  {
    this->Internal->FilePerFrameButton->setChecked(true);
  }
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::frameTransform() const
{
  if (this->Internal->RelativeButton->isChecked())
  {
    return RELATIVE_GEOPOSITION;
  }
  else if (this->Internal->AbsoluteUtmButton->isChecked())
  {
    return ABSOLUTE_GEOPOSITION_UTM;
  }
  else if (this->Internal->AbsoluteLatLonButton->isChecked())
  {
    return ABSOLUTE_GEOPOSITION_LATLON;
  }
  else
  {
    return SENSOR;
  }
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::frameMaximun() const
{
  return std::max(this->Internal->FrameStart->maximum(), this->Internal->FrameStop->maximum());
}

//-----------------------------------------------------------------------------
int lqSelectFramesDialog::frameMinimun() const
{
  return std::max(this->Internal->FrameStart->minimum(), this->Internal->FrameStop->minimum());
  ;
}

//-----------------------------------------------------------------------------
bool lqSelectFramesDialog::frameStrideVisibility() const
{
  return this->Internal->FrameStrideContainer->isVisible();
  ;
}

//-----------------------------------------------------------------------------
bool lqSelectFramesDialog::framePackVisibility() const
{
  return this->Internal->FramePackContainer->isVisible();
  ;
}

//-----------------------------------------------------------------------------
bool lqSelectFramesDialog::frameTransformVisibility() const
{
  return this->Internal->FrameTransformContainer->isVisible();
  ;
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameTransform(int frameTransform)
{
  if (frameTransform == SENSOR)
  {
    this->Internal->SensorButton->setChecked(true);
  }
  else if (frameTransform == RELATIVE_GEOPOSITION)
  {
    this->Internal->RelativeButton->setChecked(true);
  }
  else if (frameTransform == ABSOLUTE_GEOPOSITION_UTM)
  {
    this->Internal->AbsoluteUtmButton->setChecked(true);
  }
  else if (frameTransform == ABSOLUTE_GEOPOSITION_LATLON)
  {
    this->Internal->AbsoluteLatLonButton->setChecked(true);
  }
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameMinimum(int frameMin)
{
  this->Internal->FrameStart->setMinimum(frameMin);
  this->Internal->FrameStop->setMinimum(frameMin);
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameMaximum(int frameMax)
{
  this->Internal->FrameStart->setMaximum(frameMax);
  this->Internal->FrameStop->setMaximum(frameMax);
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameStrideVisibility(bool visible)
{
  this->Internal->FrameStrideContainer->setVisible(visible);
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFramePackVisibility(bool visible)
{
  this->Internal->FramePackContainer->setVisible(visible);
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::setFrameTransformVisibility(bool visible)
{
  this->Internal->FrameTransformContainer->setVisible(visible);
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);
  this->resize(this->width(), this->minimumSizeHint().height());
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::saveState()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("LidarPlugin/SelectFramesDialog/Mode", this->frameMode());
  settings->setValue("LidarPlugin/SelectFramesDialog/Start", this->frameStart());
  settings->setValue("LidarPlugin/SelectFramesDialog/Stop", this->frameStop());
  settings->setValue("LidarPlugin/SelectFramesDialog/Stride", this->frameStride());
  settings->setValue("LidarPlugin/SelectFramesDialog/Pack", this->framePack());
  settings->setValue("LidarPlugin/SelectFramesDialog/Transform", this->frameTransform());
  settings->setValue("LidarPlugin/SelectFramesDialog/Geometry", this->saveGeometry());
}

//-----------------------------------------------------------------------------
void lqSelectFramesDialog::restoreState()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  this->restoreGeometry(settings->value("LidarPlugin/SelectFramesDialog/Geometry").toByteArray());
  this->setFrameMode(settings->value("LidarPlugin/SelectFramesDialog/Mode", CURRENT_FRAME).toInt());
  this->setFrameStart(settings->value("LidarPlugin/SelectFramesDialog/Start", 0).toInt());
  this->setFrameStop(settings->value("LidarPlugin/SelectFramesDialog/Stop", 10).toInt());
  this->setFrameStride(settings->value("LidarPlugin/SelectFramesDialog/Stride", 1).toInt());
  this->setFramePack(settings->value("LidarPlugin/SelectFramesDialog/Pack", SINGLE_FILE).toInt());
  this->setFrameTransform(
    settings->value("LidarPlugin/SelectFramesDialog/Transform", SENSOR).toInt());
}
