#include "lqSensorWidget.h"
#include "ui_lqSensorWidget.h"

#include "lqLoadLidarStateReaction.h"
#include "lqSaveLidarStateReaction.h"

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqProxy.h>
#include <pqObjectBuilder.h>
#include <pqSMAdaptor.h>

#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include "vtkLidarStream.h"

#include <cassert>

//-----------------------------------------------------------------------------
lqSensorWidget::lqSensorWidget(QWidget *parent) :
  QWidget(parent),
  LidarSource(nullptr),
  PositionOrientationSource(nullptr),
  IsClosing(false),
  UI(new Ui::lqSensorWidget)
{
  this->UI->setupUi(this);

  // Clean Ui
  this->UI->lidarPort->setVisible(false);
  this->UI->posOrPort->setVisible(false);
  this->UI->toggle->setVisible(false);
  this->UI->PCAPFile->setVisible(false);
  this->UI->SensorInformation->setVisible(false);

  // create all connection
  this->connect(UI->close, SIGNAL(clicked()), this, SLOT(onClose()));
  this->connect(UI->calibrate, SIGNAL(clicked()), this, SLOT(onCalibrate()));
  this->connect(UI->SaveLidarState, SIGNAL(clicked()), this, SLOT(onSaveLidarState()));
  this->connect(UI->LoadLidarState, SIGNAL(clicked()), this, SLOT(onLoadLidarState()));

  this->setFocusPolicy(Qt::StrongFocus);

  this->SourceToDisplay = nullptr;
}

//-----------------------------------------------------------------------------
void lqSensorWidget::SetCalibrationFunction(std::function<void(pqPipelineSource* &, pqPipelineSource* &)> function)
{
  this->CalibrationFunction = function;
}

//-----------------------------------------------------------------------------
lqSensorWidget::~lqSensorWidget()
{
  delete this->UI;
}

//-----------------------------------------------------------------------------
void lqSensorWidget::SetLidarSource(pqPipelineSource* src)
{
  if (!src)
    return;

  // create lidarSource and update UI
  this->LidarSource = src;

  this->onUpdateUI();

  // Connect the source to update the UI if the name or the port change
  QObject::connect(src, SIGNAL(nameChanged(pqServerManagerModelItem*)), this, SLOT(onUpdateUI()));
  QObject::connect(src, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(onUpdateUI()));
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorWidget::GetLidarSource() const
{
  return this->LidarSource;
}

//-----------------------------------------------------------------------------
void lqSensorWidget::SetSourceToDisplay(pqPipelineSource* src)
{
  if (!src)
    return;

  this->SourceToDisplay = src;
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorWidget::GetSourceToDisplay() const
{
  return this->SourceToDisplay;
}

//-----------------------------------------------------------------------------
void lqSensorWidget::SetPositionOrientationSource(pqPipelineSource* src)
{
  // There is no obligation to have a Position Orientation source associated to the widget
  // If the user delete the Position Orientation packet interpretation
  // The PositionOrientationSource will be set to nullptr
  // That's why we allow setting a null position orientation source
  this->PositionOrientationSource = src;

  if(src)
  {
    // Connect the source to update the UI if the name or the port change
    QObject::connect(src, SIGNAL(nameChanged(pqServerManagerModelItem*)), this, SLOT(onUpdateUI()));
    QObject::connect(src, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(onUpdateUI()));
  }
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorWidget::GetPositionOrientationSource() const
{
  return this->PositionOrientationSource;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsWidgetLidarSource(pqPipelineSource *src) const
{
  return this->LidarSource == src;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsWidgetPositionOrientationSource(pqPipelineSource *src) const
{
  return this->PositionOrientationSource == src;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsWidgetSourceToDisplay(pqPipelineSource *src) const
{
  return this->SourceToDisplay == src;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsAttachedToWidget(pqPipelineSource * src) const
{
  return this->IsWidgetLidarSource(src) ||
         this->IsWidgetPositionOrientationSource(src) ||
         this->IsWidgetSourceToDisplay(src);
}

//-----------------------------------------------------------------------------
void lqSensorWidget::onCalibrate()
{
  if(this->CalibrationFunction)
  {
    this->CalibrationFunction(this->LidarSource, this->PositionOrientationSource);
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::onClose()
{
  if (this->LidarSource && !this->IsClosing)
  {
    this->IsClosing = true;
    this->deleteSource(this->LidarSource);

    if (this->PositionOrientationSource)
      this->deleteSource(this->PositionOrientationSource);

    this->deleteLater();
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::onShowHide()
{
  if (this->LidarSource && !this->IsClosing)
  {
    this->IsClosing = true;
    this->deleteSource(this->LidarSource);

    if (this->PositionOrientationSource)
      this->deleteSource(this->PositionOrientationSource);

    this->deleteLater();
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::onLoadLidarState()
{
  if (this->LidarSource)
  {
    lqLoadLidarStateReaction::LoadLidarState(this->LidarSource->getProxy());
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::onSaveLidarState()
{
  if (this->LidarSource)
  {
    lqSaveLidarStateReaction::SaveLidarState(this->LidarSource->getProxy());
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::deleteSource(pqProxy *src)
{
  if (src)
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    for (pqPipelineSource* consumer: static_cast<pqPipelineSource*>(src)->getAllConsumers())
      builder->destroy(consumer);

    builder->destroy(src);
  }

  src = nullptr;

}

//-----------------------------------------------------------------------------
void lqSensorWidget::onUpdateUI()
{
  assert(this->LidarSource);

  // Update UI with lidar information
  vtkSMProxy * lidarProxy = this->LidarSource->getProxy();
  vtkSMProperty * lidarPropCalibName = lidarProxy->GetProperty("CalibrationFileName");
  QString lidarCalibName = QString::fromStdString(vtkSMPropertyHelper(lidarPropCalibName).GetAsString());

  QString lidarName = this->LidarSource->getSMName();
  QString calibrationFileName = "Calibration Filename: " + lidarCalibName;
  this->UI->lidarName->setText(lidarName);
  this->UI->CalibrationFile->setText(calibrationFileName);

  // Update UI with Position Orientation information
  if(this->PositionOrientationSource)
  {
    this->UI->posOrName->setVisible(true);
    QString posOrName = this->PositionOrientationSource->getSMName();
    this->UI->posOrName->setText(posOrName);
  }
  else
  {
    this->UI->posOrName->setVisible(false);
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::focusInEvent(QFocusEvent*)
{
  emit selected(this);
}
