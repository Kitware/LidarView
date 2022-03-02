/*=========================================================================

   Program: ParaView
   Module:    pqVCRToolbar.h

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
#ifndef LQPLAYERCONTROLSTOOLBAR_H
#define LQPLAYERCONTROLSTOOLBAR_H

#include <QToolBar>

#include "lqapplicationcomponents_export.h"

class lqPlayerControlsController;

/// This class is a mixed of pqVCRToolbar and pqTimAnimationWidget
/// Rely on lqSensorListWidget to disable usage in 'Stream Mode'
/// WIP doxygen comment style
class LQAPPLICATIONCOMPONENTS_EXPORT lqPlayerControlsToolbar : public QToolBar
{
  Q_OBJECT

public:
  lqPlayerControlsToolbar(QWidget* parentObject=0, bool advancedOptionsForRecording=false);
  ~lqPlayerControlsToolbar();

protected Q_SLOTS:
  // UI Updates
  void setTimeRanges(double, double); // VCR Controller default, updates time ranges
  void onPlaying(bool);               // VCR Controller default

  void setFrameRanges(int, int); // updates frames ranges
  void onSpeedChanged(double speed); // Update UI on speed change
  void onToggled(bool enable); // Toggle Enable/Disable
  void onSetLiveMode(bool liveModeEnabled); // Convenience slot for inverted Toggling on livemode signal

  // UI Input signals
  void onComboSpeedSelected(int index); // Speed ComboBox changed by user

  void onTimestepChanged(); // Timestep has changed

Q_SIGNALS:
  void speedChange(double); // Requests controller to change speed

protected:
  lqPlayerControlsController* Controller;

private:
  Q_DISABLE_COPY(lqPlayerControlsToolbar)

  class pqInternals;
  pqInternals* UI;

};

#endif // LQPLAYERCONTROLSTOOLBAR_H
