#include "lqLidarCameraReaction.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqRenderView.h>

//-----------------------------------------------------------------------------
lqLidarCameraReaction::lqLidarCameraReaction(QAction *action,  pqCameraReaction::Mode mode)
  : Superclass(action, mode)
{
  this->ReactionMode = mode;
}

//-----------------------------------------------------------------------------
void lqLidarCameraReaction::onTriggered()
{
  switch (this->ReactionMode)
  {
    case RESET_CAMERA:
      this->resetCamera();
      break;

    case pqCameraReaction::Mode::RESET_POSITIVE_X:
      this->ResetLidarPositiveX();
      break;

    case pqCameraReaction::Mode::RESET_POSITIVE_Y:
      this->ResetLidarPositiveY();
      break;

    case pqCameraReaction::Mode::RESET_POSITIVE_Z:
      this->ResetLidarPositiveZ();
      break;

    case pqCameraReaction::Mode::RESET_NEGATIVE_X:
      this->ResetLidarNegativeX();
      break;

    case pqCameraReaction::Mode::RESET_NEGATIVE_Y:
      this->ResetLidarNegativeY();
      break;

    case pqCameraReaction::Mode::RESET_NEGATIVE_Z:
      this->ResetLidarNegativeZ();
      break;

    case pqCameraReaction::Mode::ZOOM_TO_DATA:
      this->zoomToData();
      break;

    case pqCameraReaction::Mode::ROTATE_CAMERA_CW:
      this->rotateCamera(-90.0);
      break;

    case pqCameraReaction::Mode::ROTATE_CAMERA_CCW:
      this->rotateCamera(90.0);
      break;
  }
}

void lqLidarCameraReaction::ResetCenterOfRotationOnLidarCenter()
{
  pqRenderView* rm = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  double center[3] = {0, 0, 0};
  rm->setCenterOfRotation(center);
  rm->render();
}

//-----------------------------------------------------------------------------
void lqLidarCameraReaction::ResetLidarPositiveX()
{
  pqCameraReaction::resetDirection(1, 0, 0, 0, 0, 1);
  this->ResetCenterOfRotationOnLidarCenter();
}

//-----------------------------------------------------------------------------
void lqLidarCameraReaction::ResetLidarNegativeX()
{
  pqCameraReaction::resetDirection(-1, 0, 0, 0, 0, 1);
  this->ResetCenterOfRotationOnLidarCenter();
}

//-----------------------------------------------------------------------------
void lqLidarCameraReaction::ResetLidarPositiveY()
{
  pqCameraReaction::resetDirection(0, 1, 0, 0, 0, 1);
  this->ResetCenterOfRotationOnLidarCenter();
}

//-----------------------------------------------------------------------------
void lqLidarCameraReaction::ResetLidarNegativeY()
{
  pqCameraReaction::resetDirection(0, -1, 0, 0, 0, 1);
  this->ResetCenterOfRotationOnLidarCenter();
}

//-----------------------------------------------------------------------------
void lqLidarCameraReaction::ResetLidarPositiveZ()
{
  pqCameraReaction::resetDirection(0, 0, 1, 0, 1, 0);
  this->ResetCenterOfRotationOnLidarCenter();
}

//-----------------------------------------------------------------------------
void lqLidarCameraReaction::ResetLidarNegativeZ()
{
  pqCameraReaction::resetDirection(0, 0, -1, 0, 1, 0);
  this->ResetCenterOfRotationOnLidarCenter();
}
