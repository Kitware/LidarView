#include "lqSensorWidget.h"
#include "ui_lqSensorWidget.h"

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqObjectBuilder.h>
#include <pqSMAdaptor.h>

#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

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
void lqSensorWidget::SetPositionOrientationSource(pqPipelineSource* src)
{
  if (!src)
    return;

  this->PositionOrientationSource = src;
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorWidget::GetPositionOrientationSource() const
{
  return this->PositionOrientationSource;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsLinkedTo(pqPipelineSource *src) const
{
  return this->LidarSource == src;
}

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
    }
    else
    {
      this->LidarSource->getProxy()->InvokeCommand("Stop");
      if (this->PositionOrientationSource)
        this->PositionOrientationSource->getProxy()->InvokeCommand("Stop");

      this->UI->toggle->setText("Start");
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

  // Update UI with sensor information
  vtkSMProxy * proxy = this->LidarSource->getProxy();
  vtkSMProperty * PropListeningAddress= proxy->GetProperty("LocalListeningAddress");
  QString listeningAddress = QString::fromStdString(vtkSMPropertyHelper(PropListeningAddress).GetAsString());
  vtkSMProperty * PropListeningPort= proxy->GetProperty("ListeningPort");
  QString listeningPort = QString::fromStdString(std::to_string(vtkSMPropertyHelper(PropListeningPort).GetAsInt()));

  this->UI->labelID->setText("LIDAR ID :" + this->LidarSource->getSMName());
  this->UI->labelIP->setText("IP/Port : " + listeningAddress +  " / " + listeningPort);

}
