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
#include "lqPlayerControlsController.h"

#include <QApplication>
#include <QtDebug>
#include <QPointer>

#include <lqSensorListWidget.h>

#include <pqAnimationScene.h>
#include <pqLiveSourceBehavior.h>
#include <pqSMAdaptor.h>

#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMIntVectorProperty.h>

// vtkAnimationScene doesn't have a enum for PLAYMODE_SNAP_TO_TIMESTEP despite this mode is implemented
constexpr int PLAYMODE_SNAP_TO_TIMESTEP = 2;

// Scene Property Helpers
namespace {
void SetProperty(QPointer<pqAnimationScene> scene, const char* property, int value)
{
  vtkSMIntVectorProperty::SafeDownCast(scene->getProxy()->GetProperty(property))->SetElements1(value);
  scene->getProxy()->UpdateProperty(property);
}

//int GetProperty(QPointer<pqAnimationScene> scene, const char* property)
//{
//  return pqSMAdaptor::getElementProperty(scene->getProxy()->GetProperty(property)).toInt();
//}
}

// Debugging helper
void debugScene(pqAnimationScene* scene)
{
  if (scene){
    QPair<double, double> range = scene->getClockTimeRange();
    std::cout << "Range: "<< range.first << " to "<< range.second << std::endl;
    std::cout << "Time : "<< scene->getAnimationTime() << std::endl;
    auto timesteps = scene->getTimeSteps(); // LITERALLY this->getServer()->getTimeKeeper()->getTimeSteps();
    double min = *std::min_element(timesteps.begin(), timesteps.end());
    double max = *std::max_element(timesteps.begin(), timesteps.end());
    std::cout << "Timesteps : "<< "0-"<< timesteps.size() << ", mM: "<< min << " to " << max << std::endl;
  }else{
    std::cout << "Scene is NULL "<< std::endl;
  }
}


//-----------------------------------------------------------------------------
lqPlayerControlsController::lqPlayerControlsController(QObject* _parent)
  : pqVCRController(_parent), speed(1.0)
{

}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::setAnimationScene(pqAnimationScene* scene)
{
  // This hardly happens if ever, aside from beginning of app
  // Regular VCR controller slot
  Superclass::setAnimationScene(scene);
  // Connects the following signals
    //QObject::connect(this->Scene, SIGNAL(tick(int))               , this, SLOT(onTick()));
    //QObject::connect(this->Scene, SIGNAL(loopChanged())           , this, SLOT(onLoopPropertyChanged()));
    //QObject::connect(this->Scene, SIGNAL(clockTimeRangesChanged()), this, SLOT(onTimeRangesChanged()));
    //QObject::connect(this->Scene, SIGNAL(beginPlay())             , this, SLOT(onBeginPlay()));
    //QObject::connect(this->Scene, SIGNAL(endPlay())               , this, SLOT(onEndPlay()));

  // Connect additional Signals:
  if(scene){
    QObject::connect(this->Scene, SIGNAL(timeStepsChanged())        , this, SLOT(onTimeStepsChanged()));
    //frameCountChanged // Not necessary, timeStepsChanged() cover this case
    //cues
    //playModeChanged()
    //animationTime (double time)
    //timeLabelChanged()
  }
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onTimeRangesChanged()
{
  if (!this->Scene)
    return;

  // Regular VCR controller onTimeRangesChanged
  Superclass::onTimeRangesChanged();
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onTimeStepsChanged()
{
  if (this->Scene)
  {
    auto timesteps = this->Scene->getTimeSteps(); // LITERALLY this->getServer()->getTimeKeeper()->getTimeSteps();
    int nframes = timesteps.size() ? timesteps.size()-1 : 0;
    Q_EMIT this->frameRanges(0, nframes );
  }
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onPause()
{
  if(!lqSensorListWidget::instance()->isInLiveSensorMode()){
    // Prevent Noisy warnings
    if (!this->Scene)
    {
      return;
    }
    // Regular VCR controller Play
    Superclass::onPause();
  }else{
    // Do not Notify Animation scene in LivestreamMode
    // Call pqLiveSourceBehavior
    bool paused = pqLiveSourceBehavior::isPaused();
    if(paused){
      pqLiveSourceBehavior::resume();
    }else{
      pqLiveSourceBehavior::pause();
    }
    // Manually notify toolbar
    Q_EMIT this->playing(paused);
  }
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onPlay()
{
  if(!lqSensorListWidget::instance()->isInLiveSensorMode()){
    // Prevent Noisy warnings
    if (!this->Scene)
    {
      return;
    }

    // Set the right play mode
    this->setPlayMode(this->speed);

    // Regular VCR controller Play
    Superclass::onPlay();
  }else{
    // Do not Notify Animation scene in LivestreamMode
    // Call pqLiveSourceBehavior
    bool paused = pqLiveSourceBehavior::isPaused();
    if(paused){
      pqLiveSourceBehavior::resume();
    }else{
      pqLiveSourceBehavior::pause();
    }
    // Manually notify toolbar
    Q_EMIT this->playing(paused);
  }
}
//-----------------------------------------------------------------------------
void lqPlayerControlsController::onSpeedChange(double speed)
{
  if(!this->Scene)
    return;
  // Update speed value
  this->speed = speed;

  // It's a bit useless to set the PlayMode here, as it is overriden each time
  // a new source with timestamps is added.
  // The override is performed in vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps
  // where the PlayMode is set to SNAP_TO_TIMESTEPS. This function is called by pqApplyBehavior::applied
  this->setPlayMode(this->speed);

  // Pause for user safety
  this->onPause();

  // Signal Speed has changed
  emit speedChange(this->speed);
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onSeekFrame(int index)
{
  if(!this->Scene)
    return;

  const auto& timesteps = this->Scene->getTimeSteps();
  if(index >=timesteps.size())
    return;

  // Set Scene Time
  this->setSceneTime(timesteps[index]) ;
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onSeekTime(double time)
{
  if(!this->Scene)
    return;

  if (time < 0)
    return;

  // Set Scene Time
  this->setSceneTime(time) ;
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::setSceneTime(double time)
{
  // This is safer, simpler for users and we have been unable to make it work without it too
  this->onPause();
  this->Scene->setAnimationTime(time);
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onNextFrame()
{
  // need to change the mode as in realtime next frame is actually t = t+1s
  SetProperty(this->Scene, "PlayMode", PLAYMODE_SNAP_TO_TIMESTEP);
  this->Scene->getProxy()->InvokeCommand("GoToNext");
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onPreviousFrame()
{
  // need to change the mode as in realtime previous frame is actually t = t-1s
  SetProperty(this->Scene, "PlayMode", PLAYMODE_SNAP_TO_TIMESTEP);
  this->Scene->getProxy()->InvokeCommand("GoToPrevious");
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::setPlayMode(double speed){
  if(!this->Scene)
    return;

  if (speed <= 0)
  {
    // There is no enum for
    SetProperty(this->Scene, "PlayMode", PLAYMODE_SNAP_TO_TIMESTEP);
  }
  else
  {
    QPair<double, double> range = this->Scene->getClockTimeRange();
    SetProperty(this->Scene, "Duration", (range.second - range.first) / this->speed);
    SetProperty(this->Scene, "PlayMode", vtkAnimationScene::PLAYMODE_REALTIME);
  }
}
