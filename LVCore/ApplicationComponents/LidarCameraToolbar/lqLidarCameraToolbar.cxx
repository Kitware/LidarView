#include "lqLidarCameraToolbar.h"
#include "ui_lqLidarCameraToolbar.h"

#include "pqActiveObjects.h"
#include "pqRenderViewSelectionReaction.h"

#include "lqLidarCameraReaction.h"

//-----------------------------------------------------------------------------
void lqLidarCameraToolbar::constructor()
{
  Ui::lqLidarCameraToolbar ui;
  ui.setupUi(this);
  new lqLidarCameraReaction(ui.actionLidarResetCamera, pqCameraReaction::RESET_CAMERA);
  new lqLidarCameraReaction(ui.actionLidarZoomToData, pqCameraReaction::ZOOM_TO_DATA);
  new lqLidarCameraReaction(ui.actionLidarPositiveX, pqCameraReaction::RESET_POSITIVE_X);
  new lqLidarCameraReaction(ui.actionLidarNegativeX, pqCameraReaction::RESET_NEGATIVE_X);
  new lqLidarCameraReaction(ui.actionLidarPositiveY, pqCameraReaction::RESET_POSITIVE_Y);
  new lqLidarCameraReaction(ui.actionLidarNegativeY, pqCameraReaction::RESET_NEGATIVE_Y);
  new lqLidarCameraReaction(ui.actionLidarPositiveZ, pqCameraReaction::RESET_POSITIVE_Z);
  new lqLidarCameraReaction(ui.actionLidarNegativeZ, pqCameraReaction::RESET_NEGATIVE_Z);
  new lqLidarCameraReaction(ui.actionLidarRotate90degCW, pqCameraReaction::ROTATE_CAMERA_CW);
  new lqLidarCameraReaction(ui.actionLidarRotate90degCCW, pqCameraReaction::ROTATE_CAMERA_CCW);

  new pqRenderViewSelectionReaction(
    ui.actionLidarZoomToBox, NULL, pqRenderViewSelectionReaction::ZOOM_TO_BOX);

  this->ZoomToDataAction = ui.actionLidarZoomToData;
  this->ZoomToDataAction->setEnabled(pqActiveObjects::instance().activeSource() != 0);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnabledState()));
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnabledState()));
}

//-----------------------------------------------------------------------------
void lqLidarCameraToolbar::updateEnabledState()
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  this->ZoomToDataAction->setEnabled(source && view);
}
