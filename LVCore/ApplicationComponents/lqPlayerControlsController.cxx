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

#include <pqAnimationScene.h>
#include <pqSMAdaptor.h>

#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMIntVectorProperty.h>

#include <pqLiveSourceBehavior.h>

// Scene Property Helpers
namespace {
void SetProperty(QPointer<pqAnimationScene> scene, const char* property, int value)
{
  dynamic_cast<vtkSMIntVectorProperty*>
      (scene->getProxy()->GetProperty(property))->SetElements1(value);
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
    QObject::connect(this->Scene, SIGNAL(timeStepsChanged())        , this, SLOT(onTimeStepsChanged()));
    //frameCountChanged // Not necessary, timeStepsChanged() cover this case
    //cues
    //playModeChanged()
    //animationTime (double time)
    //timeLabelChanged()
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onTimeRangesChanged()
{
  if (!this->Scene)
    return;

  // Set Scene Speed before signaling the change
  this->setSceneSpeed();

  // Regular VCR controller onTimeRangesChanged
  Superclass::onTimeRangesChanged();
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onTimeStepsChanged()
{
  if (this->Scene)
  {
    auto timesteps = this->Scene->getTimeSteps(); // LITERALLY this->getServer()->getTimeKeeper()->getTimeSteps();
    Q_EMIT this->frameRanges(0, timesteps.size());
  }
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onPause()
{
  // Prevent Noisy warnings
  if (!this->Scene)
  {
    return;
  }
  
  // Regular VCR controller Pause
  Superclass::onPause();
}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::onPlay()
{
  // Prevent Noisy warnings
  if (!this->Scene)
  {
    return;
  }
  
  // Regular VCR controller Pause
  Superclass::onPlay();
}
//-----------------------------------------------------------------------------
void lqPlayerControlsController::onSpeedChange(double speed)
{
  // Update speed value
  this->speed = speed;

  // Update animation mode depending on speed
  this->setSceneSpeed();
  
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
void lqPlayerControlsController::setSceneSpeed(){
  if(!this->Scene)
    return;

  if (this->speed != 0)
  {
    // REALTIME
    QPair<double, double> range = this->Scene->getClockTimeRange();
    SetProperty(this->Scene, "Duration", (range.second - range.first) / this->speed);
    this->setPlayMode(vtkAnimationScene::PLAYMODE_REALTIME);
  }
  else
  {
    // SNAP TO TIMESTEPS
    this->setPlayMode(vtkAnimationScene::PlayModes(2)); // No enum exists for mode 2 'Snap To Timesteps'.
  }

}

//-----------------------------------------------------------------------------
void lqPlayerControlsController::setPlayMode(vtkAnimationScene::PlayModes mode){
  if(!this->Scene)
    return;
  SetProperty(this->Scene, "PlayMode", mode);
}
