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
}
