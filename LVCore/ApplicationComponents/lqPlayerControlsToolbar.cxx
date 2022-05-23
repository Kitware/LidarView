/*=========================================================================

   Program: ParaView
   Module:    pqVCRToolbar.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "lqPlayerControlsToolbar.h"

#include "ui_lqPlayerControlsToolbar.h"

#include <limits>
#include <cmath>

#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QList>
#include <QPair>
#include <QString>
#include <QToolTip>

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqLiveSourceBehavior.h"
#include "pqPVApplicationCore.h"
#include "pqUndoStack.h"
#include "pqTimeKeeper.h"

#include "lqPlayerControlsController.h"
#include "lqStreamRecordReaction.h"
#include "lqSensorListWidget.h"

#include "vtkSMTimeKeeperProxy.h"

class lqPlayerControlsToolbar::pqInternals : public Ui::lqPlayerControlsToolbar
{
public:
  pqInternals():
    speedComboBox(nullptr),
    frameSlider(nullptr),
    timeSpinBox(nullptr),
    frameQSpinBox(nullptr),
    frameLabel(nullptr)
  {

  }

  QComboBox* speedComboBox;
  QSlider* frameSlider;
  QDoubleSpinBox* timeSpinBox;
  QSpinBox* frameQSpinBox;
  QLabel* frameLabel;

};

//-----------------------------------------------------------------------------
lqPlayerControlsToolbar::lqPlayerControlsToolbar(QWidget* parentObject,
                                                 bool advancedOptionsForRecording)
  : QToolBar(parentObject)
{
  this->UI = new pqInternals();
  Ui::lqPlayerControlsToolbar& ui = *this->UI;
  ui.setupUi(this);

  // ParaView VCR Controller code
  this->Controller = new lqPlayerControlsController(this);

  QObject::connect(ui.actionVCRPlay         , SIGNAL(triggered()), this->Controller, SLOT(onPlay()));
  QObject::connect(ui.actionVCRFirstFrame   , SIGNAL(triggered()), this->Controller, SLOT(onFirstFrame()));
  QObject::connect(ui.actionVCRPreviousFrame, SIGNAL(triggered()), this->Controller, SLOT(onPreviousFrame()));
  QObject::connect(ui.actionVCRNextFrame    , SIGNAL(triggered()), this->Controller, SLOT(onNextFrame()));
  QObject::connect(ui.actionVCRLastFrame    , SIGNAL(triggered()), this->Controller, SLOT(onLastFrame()));
  QObject::connect(ui.actionVCRLoop         , SIGNAL(toggled(bool)), this->Controller, SLOT(onLoop(bool)));

  QObject::connect(this->Controller, SIGNAL(enabled(bool)), ui.actionVCRPlay      , SLOT(setEnabled(bool)));
  QObject::connect(this->Controller, SIGNAL(enabled(bool)), ui.actionVCRFirstFrame, SLOT(setEnabled(bool)));
  QObject::connect(this->Controller, SIGNAL(enabled(bool)), ui.actionVCRPreviousFrame, SLOT(setEnabled(bool)));
  QObject::connect(this->Controller, SIGNAL(enabled(bool)), ui.actionVCRNextFrame, SLOT(setEnabled(bool)));
  QObject::connect(this->Controller, SIGNAL(enabled(bool)), ui.actionVCRLastFrame, SLOT(setEnabled(bool)));
  QObject::connect(this->Controller, SIGNAL(enabled(bool)), ui.actionVCRLoop     , SLOT(setEnabled(bool)));

  QObject::connect(this->Controller, SIGNAL(timeRanges(double, double)), this, SLOT(setTimeRanges(double, double)));
  QObject::connect(this->Controller, SIGNAL(frameRanges(int, int))     , this, SLOT(setFrameRanges(int, int)));
  QObject::connect(this->Controller, SIGNAL(loop(bool)), ui.actionVCRLoop, SLOT(setChecked(bool)));
  QObject::connect(this->Controller, SIGNAL(playing(bool)), this, SLOT(onPlaying(bool)));

  //CUSTOM
  QObject::connect(this->Controller, SIGNAL(speedChange(double)), this, SLOT(onSpeedChanged(double)));
  QObject::connect(this->Controller, SIGNAL(timestepChanged()), this, SLOT(onTimestepChanged()));

  // Widget Speed ComboBox
  this->insertWidget(this->UI->actionRecord, new QLabel("Speed:", this));
  this->UI->speedComboBox = new QComboBox(this);
  this->UI->speedComboBox->addItem("x 0.1"     ,0.1 );
  this->UI->speedComboBox->addItem("x 0.25"    ,0.25);
  this->UI->speedComboBox->addItem("x 0.5"     ,0.5 );
  this->UI->speedComboBox->addItem("x 1"       ,1.  );
  this->UI->speedComboBox->addItem("x 2"       ,2.  );
  this->UI->speedComboBox->addItem("x 3"       ,3.  );
  this->UI->speedComboBox->addItem("x 5"       ,5.  );
  this->UI->speedComboBox->addItem("x 10"      ,10. );
  this->UI->speedComboBox->addItem("x 20"      ,20. );
  this->UI->speedComboBox->addItem("x 100"     ,100.);
  this->UI->speedComboBox->addItem("All Frame" ,0.  );
  this->insertWidget(this->UI->actionRecord, this->UI->speedComboBox);
  QObject::connect(this->UI->speedComboBox, SIGNAL(activated(int)),
    this, SLOT(onComboSpeedSelected(int)));
  QObject::connect(this, SIGNAL(speedChange(double)), //This is the UI Update
    this->Controller, SLOT(onSpeedChange(double)));

  // Widget Seek Slider
  this->UI->frameSlider = new QSlider(Qt::Horizontal, this);
  this->insertWidget(this->UI->actionRecord, this->UI->frameSlider);
  this->connect(this->UI->frameSlider, SIGNAL(valueChanged(int)),
    this->Controller, SLOT(onSeekFrame(int)));
  QObject::connect(this->UI->frameSlider, &QSlider::valueChanged, // Show Frame Number, sliderMoved
    [&](int value) {
      QToolTip::showText(QCursor::pos(), QString("%1").arg(value), nullptr);
    });
  this->UI->frameSlider->setTracking(true);  // Make sure internal Value / Position are sync
  this->UI->frameSlider->setTickInterval(1); // Tapping the slider increments by one


  // Widget Time
  this->insertWidget(this->UI->actionRecord, new QLabel("Time", this));
  this->UI->timeSpinBox = new QDoubleSpinBox(this);
  this->insertWidget(this->UI->actionRecord, this->UI->timeSpinBox);
  this->connect(this->UI->timeSpinBox, SIGNAL(valueChanged(double)),
    this->Controller, SLOT(onSeekTime(double)));

  // Widget Frame
  this->insertWidget(this->UI->actionRecord, new QLabel("Frame", this));
  this->UI->frameQSpinBox = new QSpinBox(this);
  this->insertWidget(this->UI->actionRecord, this->UI->frameQSpinBox);
  this->connect(this->UI->frameQSpinBox, SIGNAL(valueChanged(int)),
    this->Controller, SLOT(onSeekFrame(int)));

  // Widget Frame Label
  this->UI->frameLabel = new QLabel();
  this->insertWidget(this->UI->actionRecord, this->UI->frameLabel);

  // Add the Recording Reaction
  new lqStreamRecordReaction(this->UI->actionRecord, true, advancedOptionsForRecording);

  // Toggle Toolbar based on lqSensorListWidget
  this->connect(lqSensorListWidget::instance(), SIGNAL(lidarStreamModeChanged(bool)), SLOT(onSetLiveMode(bool)));

  // Safe Init Widgets Values
  this->setTimeRanges(0,0);
  this->setFrameRanges(0,0);
  this->onSpeedChanged(1.0);

  // Init Controller
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    SIGNAL(activeSceneChanged(pqAnimationScene*)), this->Controller,
    SLOT(setAnimationScene(pqAnimationScene*)));
  this->Controller->setAnimationScene(
    pqPVApplicationCore::instance()->animationManager()->getActiveScene());

}

//-----------------------------------------------------------------------------
lqPlayerControlsToolbar::~lqPlayerControlsToolbar()
{
  delete this->UI;
  this->UI = nullptr;
}

//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::setTimeRanges(double start, double end)
{
  // start/end literally = QPair<double, double> range = this->Scene->getClockTimeRange();
  // Action First / Last Frames
  this->UI->actionVCRFirstFrame->setToolTip(QString("First Time (%1)").arg(start, 0, 'g'));
  this->UI->actionVCRLastFrame-> setToolTip(QString("Last  Time (%1)").arg(end, 0, 'g'));

  // Time Spinbox
  this->UI->timeSpinBox->setMinimum(start);
  this->UI->timeSpinBox->setMaximum(end);
}

//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::setFrameRanges(int min, int max)
{
  // min/max literally = scene->getTimeSteps();

  // Slider
  this->UI->frameSlider->setMinimum(min);
  this->UI->frameSlider->setMaximum(max);
  this->UI->frameSlider->setSliderPosition(min); //wipwip necessarry ?
  this->UI->frameSlider->setToolTip(QString("Frames %1/%2/%3").arg(min).arg(min).arg(max)); //wipwip factorize with sliderPosition() ?

  // Frame Spinbox
  this->UI->frameQSpinBox->setMinimum(min);
  this->UI->frameQSpinBox->setMaximum(max);
}
//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::onPlaying(bool playing)
{
  //PV code, set custom Play/Pause icons
  if (playing)
  {
    disconnect(this->UI->actionVCRPlay, SIGNAL(triggered()), this->Controller, SLOT(onPlay()));
    connect(this->UI->actionVCRPlay, SIGNAL(triggered()), this->Controller, SLOT(onPause()));
    this->UI->actionVCRPlay->setIcon(QIcon(":/lqResources/Icons/media-playback-pause.png"));
    this->UI->actionVCRPlay->setText("Pa&use");
  }
  else
  {
    connect(this->UI->actionVCRPlay, SIGNAL(triggered()), this->Controller, SLOT(onPlay()));
    disconnect(this->UI->actionVCRPlay, SIGNAL(triggered()), this->Controller, SLOT(onPause()));
    this->UI->actionVCRPlay->setIcon(QIcon(":/lqResources/Icons/media-playback-start.png"));
    this->UI->actionVCRPlay->setText("&Play");
  }

  // Original PV Comments
  // this becomes a behavior.
  // this->Implementation->Core->setSelectiveEnabledState(!playing);
}

//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::onSpeedChanged(double speed)
{

  // Try to select existing configuration
  int searchIndex = this->UI->speedComboBox->findData(speed);
  if(searchIndex != -1)
  {
    this->UI->speedComboBox->setCurrentIndex(searchIndex);
    return;
  }
}

//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::onComboSpeedSelected(int index)
{
  // Request Controller Speed change
  emit speedChange(this->UI->speedComboBox->itemData(index).value<double>());
}

//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::onToggled(bool enable)
{
  this->UI->actionVCRFirstFrame->setEnabled(enable);
  this->UI->actionVCRPreviousFrame->setEnabled(enable);
  this->UI->actionVCRPlay->setEnabled(true);
  this->UI->actionVCRNextFrame->setEnabled(enable);
  this->UI->actionVCRLastFrame->setEnabled(enable);
  this->UI->actionVCRLoop->setEnabled(enable);

  this->UI->speedComboBox->setEnabled(enable);
  this->UI->frameSlider->setEnabled(enable);
  this->UI->timeSpinBox->setEnabled(enable);
  this->UI->frameQSpinBox->setEnabled(enable);
  this->UI->frameLabel->setEnabled(enable);
  
}

//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::onSetLiveMode(bool liveModeEnabled)
{
  this->onToggled(!liveModeEnabled);
  
  // WIP MOVE THIS TO CONTROLLER
  if(liveModeEnabled){
    // Make LiveSource are running
    pqLiveSourceBehavior::resume();
  }else{
    // Sync button with state, It is paused when opening a new reader
    this->onPlaying(false);
  }
  
}

//-----------------------------------------------------------------------------
void lqPlayerControlsToolbar::onTimestepChanged()
{
  // Force All Widgets to update to current Time / Frame Value
  pqTimeKeeper* tk = pqApplicationCore::instance()->getActiveServer()->getTimeKeeper();
  double time = tk->getTime();
  int value   = tk->getTimeStepValueIndex(time);

  // Slider Position, no better method to sync pos / internal value
  this->UI->frameSlider->blockSignals(true);
  this->UI->frameSlider->setSliderPosition(value);
  this->UI->frameSlider->blockSignals(false);

  // Time SpinBox
  this->UI->timeSpinBox->blockSignals(true);
  this->UI->timeSpinBox->setValue(time);
  this->UI->timeSpinBox->blockSignals(false);

  // Frame SpinBox
  this->UI->frameQSpinBox->blockSignals(true);
  this->UI->frameQSpinBox->setValue(value);
  this->UI->frameQSpinBox->blockSignals(false);

  // TODO could add more tootips
}

