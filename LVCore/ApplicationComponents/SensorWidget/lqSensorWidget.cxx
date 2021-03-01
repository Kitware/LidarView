#include "lqSensorWidget.h"
#include "ui_lqSensorWidget.h"

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqObjectBuilder.h>
#include <pqSMAdaptor.h>

#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

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

  // create all connection
  this->connect(UI->toggle, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
  this->connect(UI->close, SIGNAL(clicked()), this, SLOT(onClose()));
  this->connect(UI->calibrate, SIGNAL(clicked()), this, SLOT(onCalibrate()));
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


  // create lidarStream source and update port and calibration file path
  this->LidarSource = src;

  // update toggle button state after start sensor
  this->UI->toggle->setChecked(true);
  this->UI->toggle->setText("Stop");

  this->UpdateUI();

}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorWidget::GetLidarSource() const
{
  return this->LidarSource;
}

//-----------------------------------------------------------------------------
void lqSensorWidget::SetPositionOrientationSource(pqPipelineSource* src)
{
  // There is no obligation to have a Position Orientation source associated to the widget
  // If the user delete the Position Orientation packet interpretation
  // The PositionOrientationSource will be set to nullptr
  // That's why we allow setting a null position orientation source
  this->PositionOrientationSource = src;
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
void lqSensorWidget::onCalibrate()
{
  if(this->CalibrationFunction)
  {
    this->CalibrationFunction(this->LidarSource, this->PositionOrientationSource);
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::onToggled(bool checked)
{
  if (this->LidarSource)
  {
    if (checked)
    {
      this->LidarSource->getProxy()->InvokeCommand("Start");
      if (this->PositionOrientationSource)
        this->PositionOrientationSource->getProxy()->InvokeCommand("Start");

      this->UI->toggle->setText("Stop");
      this->UI->toggle->setStyleSheet("color :red;");
    }
    else
    {
      this->LidarSource->getProxy()->InvokeCommand("Stop");
      if (this->PositionOrientationSource)
        this->PositionOrientationSource->getProxy()->InvokeCommand("Stop");

      this->UI->toggle->setText("Start");
      this->UI->toggle->setStyleSheet("color :green;");

    }
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
void lqSensorWidget::deleteSource(pqPipelineSource *src)
{
  if (src)
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

    for (pqPipelineSource* consumer: src->getAllConsumers())
      builder->destroy(consumer);

    builder->destroy(src);
  }

  src = nullptr;

}

//-----------------------------------------------------------------------------
void lqSensorWidget::UpdateUI()
{
  assert(this->LidarSource);
  // Update UI with lidar information
  vtkSMProxy * lidarProxy = this->LidarSource->getProxy();
  vtkSMProperty * lidarPropListeningPort= lidarProxy->GetProperty("ListeningPort");
  QString lidarListeningPort = QString::fromStdString(std::to_string(vtkSMPropertyHelper(lidarPropListeningPort).GetAsInt()));

  QString lidarName = "Name: " + this->LidarSource->getSMName();
  QString lidarPort = "Port: " + lidarListeningPort;
  this->UI->lidarName->setText(lidarName);
  this->UI->lidarPort->setText(lidarPort);

  // Update UI with Position Orientation information
  if(this->PositionOrientationSource)
  {
    vtkSMProxy * posOrProxy = this->PositionOrientationSource->getProxy();
    vtkSMProperty * posOrPropListeningPort= posOrProxy->GetProperty("ListeningPort");
    QString posOrlisteningPort = QString::fromStdString(std::to_string(vtkSMPropertyHelper(posOrPropListeningPort).GetAsInt()));

    QString posOrName = "Name: " + this->PositionOrientationSource->getSMName();
    QString posOrPort = "Port: " + posOrlisteningPort;
    this->UI->posOrName->setText(posOrName);
    this->UI->posOrPort->setText(posOrPort);
  }
  else
  {
    this->UI->posOrName->setText(QString(""));
    this->UI->posOrPort->setText(QString(""));
  }

}
