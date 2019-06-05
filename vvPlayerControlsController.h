/*=========================================================================

   Program: ParaView
   Module:    pqVCRController.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef VVPLAYERCONTROLSCONTROLLER_H
#define VVPLAYERCONTROLSCONTROLLER_H


#include "pqComponentsModule.h"
#include <QPointer>
#include <QObject>

class pqPipelineSource;
class pqAnimationScene;

// vvPlayerControlsController is the QObject that encapsulates the
// VCR control functionality.
// It provides a slot to set the scene that this object
// is using for animation. Typically, one would connect this
// slot to a pqAnimationManager like object which keeps track
// of the active animation scene.
class vvPlayerControlsController : public QObject
{
  Q_OBJECT
public:
  vvPlayerControlsController(QObject* parent = 0);
  virtual ~vvPlayerControlsController();

  pqAnimationScene* getAnimationScene();

signals:
  void timestepChanged();

  // emitted with playing(true) when play begins and
  // playing(false) when play ends.
  void playing(bool);

  // Fired when the enable state of the VCR control changes.
  // fired each time setAnimationScene() is called. If
  // called when a valid scene enabled(true) is fired,
  // else enabled(false).
  void enabled(bool);

  // Emitted to update the check state
  // of the loop.
  void loop(bool);

  void timeRanges(double, double);

  /// emitted when the animation begins playing.
  /// For now, playing of animation is non-undoable.
  void beginNonUndoableChanges();

  /// emitted when the animation ends playing.
  void endNonUndoableChanges();

public slots:
  // Set the animation scene. If null, the VCR control is disabled
  // (emits enabled(false)).
  void setAnimationScene(pqAnimationScene*);

  // Called when timeranges change.
  void onTimeRangesChanged();

  // Connect these signals to appropriate VCR buttons.
  void onFirstFrame();
  void onPreviousFrame();
  void onNextFrame();
  void onLastFrame();
  void onPlay();
  void onPause();
  void onLoop(bool checked);
  void onSpeedChange(double speed);

protected slots:
  void onTick();
  void onLoopPropertyChanged();
  void onBeginPlay();
  void onEndPlay();

private:
  vvPlayerControlsController(const vvPlayerControlsController&); // Not implemented.
  void operator=(const vvPlayerControlsController&); // Not implemented.

  QPointer<pqAnimationScene> Scene;
  double speed;
  double duration;
};

#endif // VVPLAYERCONTROLSCONTROLLER_H
