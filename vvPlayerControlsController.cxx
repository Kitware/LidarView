/*=========================================================================

   Program: ParaView
   Module:    pqVCRController.cxx

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
#include "vvPlayerControlsController.h"

// ParaView Server Manager includes.
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

// Qt includes.
#include <QPointer>
#include <QtDebug>
#include <QApplication>

// ParaView includes.
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "vtkAnimationScene.h"

namespace {
void SetProperty(QPointer<pqAnimationScene> scene, const char* property, int value)
{
  dynamic_cast<vtkSMIntVectorProperty*>
      (scene->getProxy()->GetProperty(property))->SetElements1(value);
  scene->getProxy()->UpdateProperty(property);
}
}

//-----------------------------------------------------------------------------
vvPlayerControlsController::vvPlayerControlsController(QObject* _parent/*=null*/)
  : QObject(_parent),
    speed(1),
    duration(0)
{
}

//-----------------------------------------------------------------------------
vvPlayerControlsController::~vvPlayerControlsController()
{
}

//-----------------------------------------------------------------------------
pqAnimationScene *vvPlayerControlsController::getAnimationScene() {
  return this->Scene.data();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Scene == scene)
    {
    return;
    }
  if (this->Scene)
    {
    QObject::disconnect(this->Scene, 0, this, 0);
    }
  this->Scene = scene;
  if (this->Scene)
    {
    QObject::connect(this->Scene, SIGNAL(tick(int)), this, SLOT(onTick()));
    QObject::connect(this->Scene, SIGNAL(loopChanged()),
      this, SLOT(onLoopPropertyChanged()));
    QObject::connect(this->Scene, SIGNAL(clockTimeRangesChanged()),
        this, SLOT(onTimeRangesChanged()));
    QObject::connect(this->Scene, SIGNAL(beginPlay()),
      this, SLOT(onBeginPlay()));
    QObject::connect(this->Scene, SIGNAL(endPlay()),
      this, SLOT(onEndPlay()));
    bool loop_checked = pqSMAdaptor::getElementProperty(
        scene->getProxy()->GetProperty("Loop")).toBool();
    emit this->loop(loop_checked);
    }

  this->onTimeRangesChanged();
  emit this->enabled (this->Scene != NULL);
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onTimeRangesChanged()
{
  if (this->Scene)
    {
    QPair<double, double> range = this->Scene->getClockTimeRange();
    emit this->timeRanges(range.first, range.second);
    this->duration = range.second - range.first;
    }
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onPlay()
{
  if (!this->Scene)
    {
    qDebug() << "No active scene. Cannot play.";
    return;
    }

  BEGIN_UNDO_EXCLUDE();

  SM_SCOPED_TRACE(CallMethod)
    .arg(this->Scene->getProxy())
    .arg("Play");

  if (speed != 0)
  {
    SetProperty(this->Scene, "Duration", this->duration / this->speed);
    SetProperty(this->Scene, "PlayMode", vtkAnimationScene::PLAYMODE_REALTIME);
  }
  else
  {
    // there is no enum for mode 2...
    SetProperty(this->Scene, "PlayMode", 2);
  }

  this->Scene->getProxy()->InvokeCommand("Play");

  // NOTE: This is a blocking call, returns only after the
  // the animation has stopped.
  END_UNDO_EXCLUDE();

  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onTick()
{
  // No need to explicitly update all views,
  // the animation scene proxy does it.

  // process the events so that the GUI remains responsive.
  QApplication::processEvents();
  emit this->timestepChanged();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onBeginPlay()
{
  emit this->playing(true);
  emit this->beginNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onEndPlay()
{
  emit this->playing(false);
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onLoopPropertyChanged()
{
  vtkSMProxy* scene = this->Scene->getProxy();
  bool loop_checked = pqSMAdaptor::getElementProperty(
    scene->GetProperty("Loop")).toBool();
  emit this->loop(loop_checked);
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onLoop(bool checked)
{
  vtkSMProxy* scene = this->Scene->getProxy();
  pqSMAdaptor::setElementProperty(scene->GetProperty("Loop"),checked);
  scene->UpdateProperty("Loop");
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onPause()
{
  if (!this->Scene)
    {
    qDebug() << "No active scene. Cannot play.";
    return;
    }
  this->Scene->getProxy()->InvokeCommand("Stop");
  SetProperty(this->Scene, "PlayMode", 2);
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onFirstFrame()
{
  emit this->beginNonUndoableChanges();
  this->Scene->getProxy()->InvokeCommand("GoToFirst");
  SM_SCOPED_TRACE(CallMethod)
    .arg(this->Scene->getProxy())
    .arg("GoToFirst");
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onPreviousFrame()
{
  emit this->beginNonUndoableChanges();
  SetProperty(this->Scene, "PlayMode", 2);
  this->Scene->getProxy()->InvokeCommand("GoToPrevious");
  SM_SCOPED_TRACE(CallMethod)
    .arg(this->Scene->getProxy())
    .arg("GoToPrevious");
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onNextFrame()
{
  emit this->beginNonUndoableChanges();
  SetProperty(this->Scene, "PlayMode", 2);
  this->Scene->getProxy()->InvokeCommand("GoToNext");
  SM_SCOPED_TRACE(CallMethod)
    .arg(this->Scene->getProxy())
    .arg("GoToNext");
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onLastFrame()
{
  emit this->beginNonUndoableChanges();
  this->Scene->getProxy()->InvokeCommand("GoToLast");
  SM_SCOPED_TRACE(CallMethod)
    .arg(this->Scene->getProxy())
    .arg("GoToLast");
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void vvPlayerControlsController::onSpeedChange(double speed)
{
  this->speed = speed;
  this->onPause();
}

