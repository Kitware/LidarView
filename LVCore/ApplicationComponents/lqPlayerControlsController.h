/*=========================================================================

   Program: LidarView
   Module:  lqPlayerControlsController.h

   Copyright (c) Kitware Inc.
   All rights reserved.

   LidarView is a free software; you can redistribute it and/or modify it
   under the terms of the LidarView license.

   See LICENSE for the full LidarView license.
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
#ifndef LQPLAYERCONTROLSCONTROLLER_H
#define LQPLAYERCONTROLSCONTROLLER_H


#include "pqComponentsModule.h"

#include <QObject>
#include <QPointer>

#include <vtkAnimationScene.h>

#include <pqVCRController.h>

#include "lqapplicationcomponents_export.h"

/*
 * lqPlayerControlsController is an upgrade of pqVCRController
 *  - RT Speed / Snap-To-Timesteps Management
 *  - Adds a Layer to pqAnimationManager with LiveSources
 *  - Helper pqAnimationScene's 'onTimeStepsChanged' sister connection to 'onTimeRangesChanged'
 *  - Seek Frame / Time features
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqPlayerControlsController : public pqVCRController
{
  Q_OBJECT
  typedef pqVCRController Superclass;

public:
  lqPlayerControlsController(QObject* parent = 0);

public slots:
  // Controls
  virtual void setAnimationScene(pqAnimationScene*) Q_DECL_OVERRIDE;
  virtual void onTimeRangesChanged() Q_DECL_OVERRIDE;
  void onTimeStepsChanged(); // Received alongside 'onTimeRangesChanged'
  virtual void onPause() Q_DECL_OVERRIDE;
  virtual void onPlay()  Q_DECL_OVERRIDE;

  void onSpeedChange(double speed);

  void onSeekFrame(int index);
  void onSeekTime (double time);

  void onPreviousFrame(); // cannot override as the method is not virtual, but not needed in our case
  void onNextFrame(); // cannot override as the method is not virtual, but not needed in our case

Q_SIGNALS:
  void speedChange(double);   // Signal speed has changed
  void frameRanges(int, int); // Tirggered alongside VCR original 'TimeRanges'

protected:
  double speed; // Store animation Speed

  // Helpers
  void setSceneTime(double time);
  void setSceneSpeed();
  void setPlayMode(double speed);

private:
  Q_DISABLE_COPY(lqPlayerControlsController)

};

#endif // LQPLAYERCONTROLSCONTROLLER_H
