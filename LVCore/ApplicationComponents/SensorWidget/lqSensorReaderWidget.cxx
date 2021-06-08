#include "lqSensorReaderWidget.h"
#include "ui_lqSensorWidget.h"

#include <pqPipelineSource.h>

#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

//-----------------------------------------------------------------------------
lqSensorReaderWidget::lqSensorReaderWidget(QWidget *parent) :
  lqSensorWidget(parent)
{
  // In reading mode, we only read the packet received on the right lidarPort.
  // This is why we want to expose it in reader mode too
  this->UI->lidarPort->setVisible(true);
}

//-----------------------------------------------------------------------------
void lqSensorReaderWidget::onUpdateUI()
{
  lqSensorWidget::onUpdateUI();

  // Update Pcap Name information
  this->UI->PCAPFile->setVisible(true);
  vtkSMProxy * lidarProxy = this->LidarSource->getProxy();
  vtkSMProperty * lidarPropPcapName = lidarProxy->GetProperty("FileName");
  QString pcapFileName = QString::fromStdString(vtkSMPropertyHelper(lidarPropPcapName).GetAsString());
  QString pcapName = "Pcap Name: " + pcapFileName;
  this->UI->PCAPFile->setText(pcapName);

  // Update UI with Lidar Port information
  vtkSMProperty * lidarPropPort= lidarProxy->GetProperty("LidarPort");
  QString lidarPort = "Port: " + QString::fromStdString(std::to_string(vtkSMPropertyHelper(lidarPropPort).GetAsInt()));
  this->UI->lidarPort->setText(lidarPort);

  // Update UI with Position Orientation information
  if(this->PositionOrientationSource)
  {
    this->UI->posOrPort->setVisible(true);
    vtkSMProxy * posOrProxy = this->PositionOrientationSource->getProxy();
    vtkSMProperty * posOrPropPort= posOrProxy->GetProperty("PositionOrientationPort");
    QString posOrPort = "Port: " + QString::fromStdString(std::to_string(vtkSMPropertyHelper(posOrPropPort).GetAsInt()));
    this->UI->posOrPort->setText(posOrPort);
  }
  else
  {
    this->UI->posOrPort->setVisible(false);
  }
}

QString lqSensorReaderWidget::GetExplanationOnUI()
{
  QString caption = "This widget displays all readers currently opened.\n\
For each sensor, you can see its name, port, pcap and calibration file.\n\
You can use the [calibrate] button to modify some options (translation, ...)";
  return caption;
}
