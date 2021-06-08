#include "lqSensorStreamWidget.h"
#include "ui_lqSensorWidget.h"

#include <pqPipelineSource.h>

#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include "vtkLidarStream.h"

//-----------------------------------------------------------------------------
lqSensorStreamWidget::lqSensorStreamWidget(QWidget *parent) :
  lqSensorWidget(parent)
{
  this->UI->lidarPort->setVisible(true);
  this->UI->toggle->setVisible(true);

  this->connect(UI->toggle, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
}

//-----------------------------------------------------------------------------
void lqSensorStreamWidget::onToggled(bool checked)
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
void lqSensorStreamWidget::onUpdateUI()
{
  lqSensorWidget::onUpdateUI();

  // Update UI with lidar information
  vtkSMProxy * lidarProxy = this->LidarSource->getProxy();
  vtkSMProperty * lidarPropListeningPort= lidarProxy->GetProperty("ListeningPort");
  QString lidarListeningPort = QString::fromStdString(std::to_string(vtkSMPropertyHelper(lidarPropListeningPort).GetAsInt()));

  QString lidarPort = "Port: " + lidarListeningPort;
  this->UI->lidarPort->setText(lidarPort);

  // Update UI with Position Orientation information
  if(this->PositionOrientationSource)
  {
    this->UI->posOrPort->setVisible(true);
    vtkSMProxy * posOrProxy = this->PositionOrientationSource->getProxy();
    vtkSMProperty * posOrPropListeningPort= posOrProxy->GetProperty("ListeningPort");
    QString posOrlisteningPort = QString::fromStdString(std::to_string(vtkSMPropertyHelper(posOrPropListeningPort).GetAsInt()));

    QString posOrPort = "Port: " + posOrlisteningPort;
    this->UI->posOrPort->setText(posOrPort);
  }
  else
  {
    this->UI->posOrPort->setVisible(false);
  }

  // Get the Sensor Information with the vtkLidarStream
  // We can not get the sensor information using the proxy
  // because this is not working if the method called have arguments (even if we don't want to specfy it)
  // And we want to have the shortVersion = True.
  vtkLidarStream * lidarStream = vtkLidarStream::SafeDownCast(lidarProxy->GetClientSideObject());
  if(!lidarStream)
  {
    return;
  }
  QString lidarSensorInfo = QString::fromStdString(lidarStream->GetSensorInformation(true));
  QString sensorInfo = "Sensor Information: " + lidarSensorInfo;
  this->UI->SensorInformation->setVisible(true);
  this->UI->SensorInformation->setText(sensorInfo);

}

//-----------------------------------------------------------------------------
QString lqSensorStreamWidget::GetExplanationOnUI()
{
  QString caption = "This widget displays all sensors currently opened.\n\
For each sensor, you can see its name, port, calibration file, ...\n\
You can use the [calibrate] button to modify some options (translation, ...)\n\
You can use the [start]/[stop] button to manage the stream.";
  return caption;
}
