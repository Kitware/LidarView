/*=========================================================================

  Program: LidarView
  Module:  lqLivePlayerWidget.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqLivePlayerWidget_h
#define lqLivePlayerWidget_h

#include "lqApplicationComponentsModule.h"

#include "lqLiveVCRController.h"

#include <QWidget>

#include <memory> // for unique_ptr

class pqAnimationScene;
class lqStreamRecordController;

/**
 * lqLivePlayerWidget is the main widget for the Lidar Player DockWidget.
 *
 * @sa lqLiveVCRController
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqLivePlayerWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  lqLivePlayerWidget(QWidget* parent = nullptr);
  ~lqLivePlayerWidget() override;

  /**
   * Change play mode, can either be EMULATED_TIME or ALL_FRAMES.
   * Note that if EMULATED_TIME is choosen the speed is automatically
   * determined with last speed used.
   */
  void changeReaderMode(lqLiveVCRController::PlayMode mode);

  /**
   * Get a pointer on the record controller object.
   * Note that the player keep the ownership of the object, and it should be
   * only use to connect to recorder signals.
   */
  const lqStreamRecordController* getRecordController() const;

private Q_SLOTS:
  /**
   * Linked to speed selection ComboBox.
   */
  void onSpeedSelected(int index);

  ///@{
  /**
   * Triggered by lqLiveVCRController signals, update widget UI accordingly.
   */
  void updateState(lqLiveVCRController::PlayMode mode);
  void setTimeRanges(double min, double max);
  void setFrameRanges(int first, int last);
  void onPlaying(bool isPlaying, bool reverse);
  void onTimestepChanged(double time);
  ///@}

private:
  Q_DISABLE_COPY(lqLivePlayerWidget)
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
