/*=========================================================================

  Program: LidarView
  Module:  lqLiveVCRController.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqLiveVCRController_h
#define lqLiveVCRController_h

#include <QObject>
#include <QPointer>

#include <pqVCRController.h>

#include "lqApplicationComponentsModule.h"

/**
 * lqLiveVCRController extends the functionality of pqVCRController by supporting multiple
 * playback modes that are used in LidarView applications:
 *  - `ALL_FRAMES`: Standard playback mode from `pqVCRController`, every frame is played
 *     sequentially.
 *  - `EMULATED_TIME`: Playback simulates real-time speed. @sa vtkEmulatedTimeAlgorithm
 *  - `STREAM`: Continuously processes incoming data from LiveSource as it becomes available.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqLiveVCRController : public pqVCRController
{
  Q_OBJECT
  typedef pqVCRController Superclass;

public:
  lqLiveVCRController(QObject* parent = nullptr);

  enum PlayMode
  {
    DISABLED = 0,
    ALL_FRAMES,
    EMULATED_TIME,
    STREAM
  };

  /**
   * Change reader mode, can either be ALL_FRAMES (frame priority)
   * or EMULATED_TIME (time priority).
   */
  void setReaderMode(PlayMode mode);

  /**
   * Get EMULATED_TIME mode current speed.
   */
  static double getCurrentSpeed();

  /**
   * Get current scene time.
   * Note: the underlying implementation depends on the selected mode.
   * Return 0 for PlayMode::DISABLED and PlayMode::STREAM.
   */
  double getSceneTime();

public Q_SLOTS:
  /**
   * Set the animation scene. If null, the VCR control is disabled.
   */
  void setAnimationScene(pqAnimationScene*) override;

  ///@{
  /**
   * Slots inherited from pqVCRController should be connected to appropriate VCR buttons.
   *
   * Note: In `EMULATED_TIME` mode, the `onPreviousFrame()` and `onNextFrame()` slots will
   * advance or rewind the time by an amount that is determined by the current playback speed.
   */
  void onPlay() override;
  void onPause() override;
  void onFirstFrame() override;
  void onPreviousFrame() override;
  void onNextFrame() override;
  void onLastFrame() override;
  ///@}

  /**
   * Change EmulatedTime live source speed.
   */
  void onSpeedChange(double speed);

  ///@{
  /**
   * Change scene time by frame index or scene time.
   *
   * @sa setSceneTime
   */
  void onSeekFrame(int index);
  void onSeekTime(double time);
  ///@}

Q_SIGNALS:
  /**
   * Emitted for each data updated.
   * Should be used instead of pqVCRController::timestepChanged, as it does not
   * emit for LiveSource updates.
   */
  void timeChanged(double time);

  /**
   * Emitted when the frame ranges is updated.
   * @sa pqVCRController::timeRanges()
   */
  void frameRanges(int first, int last);

  /**
   * Emitted when a the reader mode is changed, a LiveSource (stream) is detected
   * or VCR controls are disabled.
   */
  void modeChanged(PlayMode mode);

private Q_SLOTS:
  void onSourceAdded(pqPipelineSource* src);
  void onSourceRemoved(pqPipelineSource* src);

private:
  Q_DISABLE_COPY(lqLiveVCRController)

  PlayMode getCurrentMode();
  void setSceneTime(double time);

private:
  PlayMode ReaderMode = PlayMode::ALL_FRAMES;
  bool IsStream = false;
  double LastSceneTime = 0.;
};

#endif // lqLiveVCRController_H
