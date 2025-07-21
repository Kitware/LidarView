/*=========================================================================

  Program: LidarView
  Module:  lqLivePlayerWidget.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLivePlayerWidget.h"
#include "ui_lqLivePlayerWidget.h"

#include "lqLiveVCRController.h"
#include "lqStreamRecordController.h"
#include "lqTimestampLineEdit.h"

#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqApplicationCore.h>
#include <pqPVApplicationCore.h>
#include <pqServer.h>
#include <pqSpinBox.h>
#include <pqTimeKeeper.h>

#include <vtkSetGet.h>

#include <QToolButton>

#include <limits>
#include <map>

namespace
{
const std::map<double, QString> SpeedOptions = {
  { 0., "All Frames" },
  { 0.2, "x 0.2" },
  { 0.5, "x 0.5" },
  { 1., "x 1" },
  { 2., "x 2" },
  { 3., "x 3" },
  { 5., "x 5" },
  { 10., "x 10" },
};

}

struct lqLivePlayerWidget::pqInternals
{
  Ui::LivePlayerWidget Ui;
  lqLiveVCRController* Controller;
  lqStreamRecordController* Recorder;

  pqInternals(lqLivePlayerWidget* self)
    : Controller(new lqLiveVCRController(self))
    , Recorder(new lqStreamRecordController(self))
  {
    this->Ui.setupUi(self);
    for (const auto& it : ::SpeedOptions)
    {
      this->Ui.VCRSpeed->addItem(it.second, it.first);
    }
    // When slider is dragged update values.
    this->Ui.FrameSlider->setTracking(true);
    this->Ui.FrameSlider->setTickInterval(1);
  }
};

//-----------------------------------------------------------------------------
lqLivePlayerWidget::lqLivePlayerWidget(QWidget* parent)
  : Superclass(parent)
  , Internals(new lqLivePlayerWidget::pqInternals(this))
{
  auto& controller = this->Internals->Controller;
  auto& recorder = this->Internals->Recorder;
  auto& ui = this->Internals->Ui;

  using lqVCR = lqLiveVCRController;
  using lqRec = lqStreamRecordController;
  QObject::connect(ui.VCRFirstFrame, &QToolButton::pressed, controller, &lqVCR::onFirstFrame);
  QObject::connect(ui.VCRPreviousFrame, &QToolButton::pressed, controller, &lqVCR::onPreviousFrame);
  QObject::connect(ui.VCRPlay, &QToolButton::clicked, controller, &lqVCR::onPlay);
  QObject::connect(ui.VCRNextFrame, &QToolButton::pressed, controller, &lqVCR::onNextFrame);
  QObject::connect(ui.VCRLastFrame, &QToolButton::pressed, controller, &lqVCR::onLastFrame);
  QObject::connect(ui.VCRLoop, &QToolButton::clicked, controller, &lqVCR::onLoop);
  QObject::connect(ui.VCRRecord, &QToolButton::clicked, recorder, &lqRec::onRecordStream);

  QObject::connect(ui.FrameSlider, &QSlider::valueChanged, controller, &lqVCR::onSeekFrame);
  QObject::connect(
    ui.TimeLineEdit, &lqTimestampLineEdit::timestampUpdated, controller, &lqVCR::onSeekTime);
  QObject::connect(ui.FrameSpinBox,
    &pqSpinBox::valueChangedAndEditingFinished,
    [&controller, &ui]() { controller->onSeekFrame(ui.FrameSpinBox->value()); });
  QObject::connect(ui.VCRSpeed,
    QOverload<int>::of(&QComboBox::currentIndexChanged),
    this,
    &lqLivePlayerWidget::onSpeedSelected);

  QObject::connect(controller, &lqVCR::modeChanged, this, &lqLivePlayerWidget::updateState);
  QObject::connect(controller, &lqVCR::timeRanges, this, &lqLivePlayerWidget::setTimeRanges);
  QObject::connect(controller, &lqVCR::frameRanges, this, &lqLivePlayerWidget::setFrameRanges);
  QObject::connect(controller, &lqVCR::playing, this, &lqLivePlayerWidget::onPlaying);
  QObject::connect(controller, &lqVCR::loop, ui.VCRLoop, &QToolButton::setChecked);
  QObject::connect(controller, &lqVCR::timeChanged, this, &lqLivePlayerWidget::onTimestepChanged);
  QObject::connect(recorder, &lqRec::recording, ui.VCRRecord, &QToolButton::setChecked);
  this->updateState(lqLiveVCRController::DISABLED);

  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  QObject::connect(
    mgr, &pqAnimationManager::activeSceneChanged, controller, &lqVCR::setAnimationScene);
  controller->setAnimationScene(mgr->getActiveScene());
}

//-----------------------------------------------------------------------------
lqLivePlayerWidget::~lqLivePlayerWidget() = default;

//-----------------------------------------------------------------------------
void lqLivePlayerWidget::changeReaderMode(lqLiveVCRController::PlayMode mode)
{
  auto& ui = this->Internals->Ui;
  bool isEmulatedTime = mode == lqLiveVCRController::EMULATED_TIME;
  double key = isEmulatedTime ? lqLiveVCRController::getCurrentSpeed() : 0.;
  auto found = ::SpeedOptions.find(key);
  // Get the ComboBox index. If the key wasn't found default to x 1.
  size_t idx = found != ::SpeedOptions.end() ? std::distance(::SpeedOptions.begin(), found) : 1.;
  ui.VCRSpeed->setCurrentIndex(idx);
}

//-----------------------------------------------------------------------------
void lqLivePlayerWidget::updateState(lqLiveVCRController::PlayMode mode)
{
  auto& ui = this->Internals->Ui;
  bool enabled = mode != lqLiveVCRController::DISABLED;
  bool isAllFrame = mode == lqLiveVCRController::ALL_FRAMES;
  bool isStreaming = mode == lqLiveVCRController::STREAM;
  double time = this->Internals->Controller->getSceneTime();
  ui.VCRFirstFrame->setEnabled(enabled && !isStreaming);
  ui.VCRPreviousFrame->setEnabled(enabled && !isStreaming && time != ui.FrameSlider->minimum());
  ui.VCRPlay->setEnabled(enabled);
  ui.VCRNextFrame->setEnabled(enabled && !isStreaming && time != ui.FrameSlider->maximum());
  ui.VCRLastFrame->setEnabled(enabled && !isStreaming);
  ui.VCRLoop->setEnabled(enabled && isAllFrame);
  ui.VCRSpeed->setEnabled(enabled && !isStreaming);
  if (!enabled)
  {
    this->setTimeRanges(0., 0.);
    this->setFrameRanges(0, 1);
  }
  ui.TimeLineEdit->setEnabled(enabled && !isStreaming);
  ui.FrameSpinBox->setEnabled(enabled && !isStreaming);
  ui.FrameSlider->setEnabled(enabled && !isStreaming);
  ui.VCRRecord->setEnabled(isStreaming);
}

//-----------------------------------------------------------------------------
void lqLivePlayerWidget::setTimeRanges(double start, double end)
{
  auto& ui = this->Internals->Ui;
  ui.VCRFirstFrame->setToolTip(
    "Go to first frame (" + lqTimestampLineEdit::convertTimestampToDate(start) + ")");
  ui.VCRLastFrame->setToolTip(
    "Go to last frame (" + lqTimestampLineEdit::convertTimestampToDate(end) + ")");

  if (end < start)
  {
    end = start;
  }
  ui.TimeLineEdit->setValidatorRange(start, end);
  ui.TimeLineEdit->setTimestamp(start);

  // This line ensures the scene resets when a lidar reader is deleted.
  // Without this reset, deleting the last lidar source while the player is running
  // could cause an infinite loop bug.
  this->Internals->Controller->onSeekFrame(start);
}

//-----------------------------------------------------------------------------
void lqLivePlayerWidget::setFrameRanges(int min, int max)
{
  auto& ui = this->Internals->Ui;

  ui.FrameSlider->setMinimum(min);
  ui.FrameSlider->setMaximum(max);
  ui.FrameSlider->setSliderPosition(min);

  ui.FrameSpinBox->setMinimum(min);
  ui.FrameSpinBox->setMaximum(max);

  // When a new data is open go to first frame.
  this->Internals->Controller->onSeekFrame(min);
}

//-----------------------------------------------------------------------------
void lqLivePlayerWidget::onPlaying(bool isPlaying, bool vtkNotUsed(reverse))
{
  auto& controller = this->Internals->Controller;
  auto& ui = this->Internals->Ui;
  using lqVCR = lqLiveVCRController;
  if (isPlaying)
  {
    QObject::disconnect(ui.VCRPlay, &QToolButton::clicked, controller, &lqVCR::onPlay);
    QObject::connect(ui.VCRPlay, &QToolButton::clicked, controller, &lqVCR::onPause);
    ui.VCRPlay->setIcon(QIcon(":/pqWidgets/Icons/pqVcrPause.svg"));
    ui.VCRPlay->setText("Pa&use");
    ui.VCRPlay->setToolTip("Pause");
  }
  else
  {
    QObject::connect(ui.VCRPlay, &QToolButton::clicked, controller, &lqVCR::onPlay);
    QObject::disconnect(ui.VCRPlay, &QToolButton::clicked, controller, &lqVCR::onPause);
    ui.VCRPlay->setIcon(QIcon(":/pqWidgets/Icons/pqVcrPlay.svg"));
    ui.VCRPlay->setText("&Play");
    ui.VCRPlay->setToolTip("Play");
  }
  ui.VCRPlay->setShortcut(QKeySequence("Space"));
}

//-----------------------------------------------------------------------------
void lqLivePlayerWidget::onSpeedSelected(int index)
{
  // Request Controller Speed change
  auto& ui = this->Internals->Ui;
  double speedRequested = ui.VCRSpeed->itemData(index).toDouble();
  if (speedRequested == 0.)
  {
    this->Internals->Controller->setReaderMode(lqLiveVCRController::ALL_FRAMES);
    ui.VCRPreviousFrame->setToolTip("Previous frame");
    ui.VCRPreviousFrame->setIcon(QIcon(":/pqWidgets/Icons/pqVcrBack.svg"));
    ui.VCRNextFrame->setToolTip("Next frame");
    ui.VCRNextFrame->setIcon(QIcon(":/pqWidgets/Icons/pqVcrForward.svg"));
  }
  else
  {
    this->Internals->Controller->setReaderMode(lqLiveVCRController::EMULATED_TIME);
    this->Internals->Controller->onSpeedChange(speedRequested);
    ui.VCRPreviousFrame->setToolTip("Rewind by " + QString::number(speedRequested) + " seconds");
    ui.VCRPreviousFrame->setIcon(QIcon(":/lqResources/Icons/lqVcrRewindTime.svg"));
    ui.VCRNextFrame->setToolTip("Jump " + QString::number(speedRequested) + " seconds");
    ui.VCRNextFrame->setIcon(QIcon(":/lqResources/Icons/lqVcrForwardTime.svg"));
  }
}

//-----------------------------------------------------------------------------
void lqLivePlayerWidget::onTimestepChanged(double timestep)
{
  auto& ui = this->Internals->Ui;
  pqTimeKeeper* tk = pqApplicationCore::instance()->getActiveServer()->getTimeKeeper();
  int frameIndex = tk->getTimeStepValueIndex(timestep);

  // Slider Position, no better method to sync pos / internal value
  ui.FrameSlider->blockSignals(true);
  ui.FrameSlider->setSliderPosition(frameIndex);
  ui.FrameSlider->blockSignals(false);

  ui.TimeLineEdit->setTimestamp(timestep);

  // Frame SpinBox
  ui.FrameSpinBox->blockSignals(true);
  ui.FrameSpinBox->setValue(frameIndex);
  ui.FrameSpinBox->blockSignals(false);

  ui.VCRPreviousFrame->setEnabled(frameIndex != ui.FrameSlider->minimum());
  ui.VCRNextFrame->setEnabled(frameIndex != ui.FrameSlider->maximum());
}

//-----------------------------------------------------------------------------
const lqStreamRecordController* lqLivePlayerWidget::getRecordController() const
{
  return this->Internals->Recorder;
}
