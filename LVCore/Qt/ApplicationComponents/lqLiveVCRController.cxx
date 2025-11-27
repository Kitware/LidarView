/*=========================================================================

  Program: LidarView
  Module:  lqLiveVCRController.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLiveVCRController.h"

#include <QApplication>
#include <QPointer>
#include <QtDebug>

#include <pqAnimationScene.h>
#include <pqApplicationCore.h>
#include <pqLiveSourceItem.h>
#include <pqLiveSourceManager.h>
#include <pqPVApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqProxy.h>
#include <pqSMAdaptor.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqTimeKeeper.h>

#include <vtkCompositeAnimationPlayer.h>
#include <vtkPVDataInformation.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>

#include "vtkSMLidarReaderProxy.h"
#include "vtkSMLidarStreamProxy.h"

#include <QList>

namespace
{
void UpdateEmulatedCurrentTime(double timestamp)
{
  pqLiveSourceManager* lsm = pqPVApplicationCore::instance()->liveSourceManager();
  pqTimeKeeper* tk = pqApplicationCore::instance()->getActiveServer()->getTimeKeeper();
  auto range = tk->getTimeRange();
  timestamp = std::clamp(timestamp, range.first, range.second);
  lsm->setEmulatedCurrentTime(timestamp);
  // If the emulated time algorithm is paused, live sources must be refreshed manually
  if (lsm->isEmulatedTimePaused())
  {
    pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqProxy*> proxies = model->findItems<pqProxy*>();
    for (auto& proxy : proxies)
    {
      pqLiveSourceItem* lsItem = lsm->getLiveSourceItem(proxy->getProxy());
      if (lsItem)
      {
        Q_EMIT lsItem->refreshSource();
      }
    }
  }
}
}

//-----------------------------------------------------------------------------
lqLiveVCRController::lqLiveVCRController(QObject* _parent)
  : pqVCRController(_parent)
{
  auto forwardEnabled = [this](bool enabled)
  { Q_EMIT this->modeChanged(enabled ? this->getCurrentMode() : PlayMode::DISABLED); };
  this->connect(this, &pqVCRController::enabled, forwardEnabled);

  // If a stream is started change the current mode.
  auto streamModeChanged = [this]()
  {
    if (this->getCurrentMode() == PlayMode::STREAM)
    {
      // We want the stream to start automatically.
      this->onPlay();
    }
    else
    {
      // Pause potentially running stream / avoid conflict with emulated time source
      pqLiveSourceManager* lsm = pqPVApplicationCore::instance()->liveSourceManager();
      lsm->pause();
      Q_EMIT this->playing(false, false);
    }
  };
  this->connect(this, &lqLiveVCRController::modeChanged, streamModeChanged);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(
    smmodel, &pqServerManagerModel::sourceAdded, this, &lqLiveVCRController::onSourceAdded);
  this->connect(
    smmodel, &pqServerManagerModel::sourceRemoved, this, &lqLiveVCRController::onSourceRemoved);

  // Emit timeChanged signal when data is updated.
  auto onSourceTimeUpdated = [this]()
  {
    double currentTime = this->getSceneTime();
    if (this->LastSceneTime != currentTime)
    {
      this->LastSceneTime = currentTime;
      Q_EMIT this->timeChanged(currentTime);
    }
  };
  this->connect(smmodel, &pqServerManagerModel::dataUpdated, this, onSourceTimeUpdated);

  // Automatically stop the player if no frames left.
  auto stopEmulatedTime = [this]()
  {
    if (this->getCurrentMode() == PlayMode::EMULATED_TIME)
    {
      double currentTime = this->getSceneTime();
      pqTimeKeeper* tk = pqApplicationCore::instance()->getActiveServer()->getTimeKeeper();
      auto range = tk->getTimeRange();
      if (currentTime >= range.second)
      {
        this->setSceneTime(range.second);
        this->onPause();
      }
    }
  };
  this->connect(smmodel, &pqServerManagerModel::dataUpdated, this, stopEmulatedTime);
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::setReaderMode(PlayMode mode)
{
  if (mode == this->ReaderMode || (mode != PlayMode::ALL_FRAMES && mode != PlayMode::EMULATED_TIME))
  {
    return;
  }
  this->onPause();
  double currentTime = this->getSceneTime();
  this->ReaderMode = mode;
  Q_EMIT this->modeChanged(mode);
  this->setSceneTime(currentTime);
  this->onPause();
}

//-----------------------------------------------------------------------------
lqLiveVCRController::PlayMode lqLiveVCRController::getCurrentMode()
{
  if (!this->getAnimationScene())
  {
    return PlayMode::DISABLED;
  }
  return this->IsStream ? PlayMode::STREAM : this->ReaderMode;
}

//-----------------------------------------------------------------------------
double lqLiveVCRController::getCurrentSpeed()
{
  pqLiveSourceManager* lsm = pqPVApplicationCore::instance()->liveSourceManager();
  return lsm->getEmulatedSpeedMultiplier();
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::setAnimationScene(pqAnimationScene* scene)
{
  if (this->getAnimationScene() == scene)
  {
    return;
  }
  Superclass::setAnimationScene(scene);
  if (scene)
  {
    auto onTimeStepsChanged = [this]()
    {
      auto timesteps = this->getAnimationScene()->getTimeSteps();
      int nframes = timesteps.empty() ? 0 : timesteps.size() - 1;
      Q_EMIT this->frameRanges(0, nframes);
    };
    this->connect(scene, &pqAnimationScene::timeStepsChanged, onTimeStepsChanged);
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onPlay()
{
  pqLiveSourceManager* lsm = pqPVApplicationCore::instance()->liveSourceManager();

  // If no more frame left - restart the emulated time.
  if (this->getCurrentMode() == PlayMode::EMULATED_TIME)
  {
    double currentTime = this->getSceneTime();
    pqTimeKeeper* tk = pqApplicationCore::instance()->getActiveServer()->getTimeKeeper();
    auto range = tk->getTimeRange();
    if (currentTime >= range.second)
    {
      ::UpdateEmulatedCurrentTime(range.first);
    }
  }

  switch (this->getCurrentMode())
  {
    case PlayMode::ALL_FRAMES:
      // Regular VCR controller Play
      Superclass::onPlay();
      break;

    case PlayMode::EMULATED_TIME:
    case PlayMode::STREAM:
      lsm->resume();
      // Manually notify toolbar
      Q_EMIT this->playing(true, false);
      break;

    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onPause()
{
  pqLiveSourceManager* lsm = pqPVApplicationCore::instance()->liveSourceManager();

  switch (this->getCurrentMode())
  {
    case PlayMode::ALL_FRAMES:
      // Regular VCR controller Pause
      Superclass::onPause();
      break;

    case PlayMode::EMULATED_TIME:
    case PlayMode::STREAM:
      lsm->pause();
      // Manually notify toolbar
      Q_EMIT this->playing(false, false);
      break;

    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onFirstFrame()
{
  switch (this->getCurrentMode())
  {
    case PlayMode::ALL_FRAMES:
      Superclass::onFirstFrame();
      break;

    case PlayMode::EMULATED_TIME:
    {
      auto timesteps = this->getAnimationScene()->getTimeSteps();
      if (!timesteps.isEmpty())
      {
        ::UpdateEmulatedCurrentTime(timesteps.front());
      }
      break;
    }

    default:
      break;
  }
}
//-----------------------------------------------------------------------------
void lqLiveVCRController::onPreviousFrame()
{
  switch (this->getCurrentMode())
  {
    case PlayMode::ALL_FRAMES:
      Superclass::onPreviousFrame();
      break;

    case PlayMode::EMULATED_TIME:
      ::UpdateEmulatedCurrentTime(this->getSceneTime() - lqLiveVCRController::getCurrentSpeed());
      break;

    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onNextFrame()
{
  switch (this->getCurrentMode())
  {
    case PlayMode::ALL_FRAMES:
      Superclass::onNextFrame();
      break;

    case PlayMode::EMULATED_TIME:
      ::UpdateEmulatedCurrentTime(this->getSceneTime() + lqLiveVCRController::getCurrentSpeed());
      break;

    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onLastFrame()
{
  switch (this->getCurrentMode())
  {
    case PlayMode::ALL_FRAMES:
      Superclass::onLastFrame();
      break;

    case PlayMode::EMULATED_TIME:
    {
      auto timesteps = this->getAnimationScene()->getTimeSteps();
      if (!timesteps.isEmpty())
      {
        ::UpdateEmulatedCurrentTime(timesteps.back());
      }
      break;
    }

    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onSpeedChange(double speed)
{
  if (this->getCurrentMode() != PlayMode::EMULATED_TIME)
  {
    return;
  }

  // Pause for user safety
  this->onPause();
  pqLiveSourceManager* lsm = pqPVApplicationCore::instance()->liveSourceManager();
  lsm->setEmulatedSpeedMultiplier(speed);
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onSeekFrame(int index)
{
  if (!this->getAnimationScene())
  {
    return;
  }

  const auto& timesteps = this->getAnimationScene()->getTimeSteps();
  if (index < timesteps.size())
  {
    this->setSceneTime(timesteps[index]);
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onSeekTime(double time)
{
  if (time >= 0)
  {
    this->setSceneTime(time);
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::setSceneTime(double time)
{
  this->onPause();
  switch (this->getCurrentMode())
  {
    case PlayMode::ALL_FRAMES:
      this->getAnimationScene()->setAnimationTime(time);
      break;

    case PlayMode::EMULATED_TIME:
      ::UpdateEmulatedCurrentTime(time);
      break;

    default:
      break;
  }
}

//-----------------------------------------------------------------------------
double lqLiveVCRController::getSceneTime()
{
  if (this->getCurrentMode() == PlayMode::EMULATED_TIME)
  {
    pqLiveSourceManager* lsm = pqPVApplicationCore::instance()->liveSourceManager();
    return lsm->getEmulatedCurrentTime();
  }
  if (this->getCurrentMode() == PlayMode::ALL_FRAMES)
  {
    pqTimeKeeper* tk = pqApplicationCore::instance()->getActiveServer()->getTimeKeeper();
    return tk->getTime();
  }
  return 0.;
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onSourceAdded(pqPipelineSource* src)
{
  bool isStreamLidarProxy = vtkSMLidarStreamProxy::SafeDownCast(src->getProxy()) != nullptr;
  if (!this->IsStream && isStreamLidarProxy)
  {
    this->IsStream = true;
    Q_EMIT this->modeChanged(this->getCurrentMode());
  }
}

//-----------------------------------------------------------------------------
void lqLiveVCRController::onSourceRemoved(pqPipelineSource* src)
{
  // Check if the proxy removed was a stream
  bool isStreamLidarProxy = vtkSMLidarStreamProxy::SafeDownCast(src->getProxy()) != nullptr;
  if (this->IsStream && isStreamLidarProxy)
  {
    // Check if another stream exist in the pipeline.
    bool hasStream = false;
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    Q_FOREACH (pqPipelineSource* item, smmodel->findItems<pqPipelineSource*>())
    {
      if (vtkSMLidarStreamProxy::SafeDownCast(item->getProxy()) != nullptr)
      {
        hasStream = true;
        break;
      }
    }

    if (!hasStream)
    {
      this->IsStream = false;
      Q_EMIT this->modeChanged(this->getCurrentMode());
    }
  }
}
