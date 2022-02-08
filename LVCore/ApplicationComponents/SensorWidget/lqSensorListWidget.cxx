#include "lqSensorListWidget.h"

#include <algorithm>

#include "lqHelper.h"
#include "lqSensorStreamWidget.h"
#include "lqSensorReaderWidget.h"
#include "vtkTrailingFrame.h"

// QT includes.
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QScrollArea>

// ParaView Server Manager includes.
#include <vtkSMProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkPVXMLElement.h>
#include <vtkLidarStream.h>

// ParaView includes.
#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqSMAdaptor.h>
#include <pqServerManagerModel.h>
#include <pqServerManagerModelItem.h>
#include <pqComponentsModule.h>
#include <pqView.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
  class lqSensorListWidget
  {
  public:
    QVBoxLayout* panelLayout;
    QScrollArea* scrollArea;
    QWidget* scrollAreaWidgetContents;

    QVBoxLayout* verticalLayout;
    QVBoxLayout* sensorListLayout;
    QSpacerItem* verticalSpacer;

    QLabel* caption;

    void setupUi(QWidget *parent)
    {
        if (parent->objectName().isEmpty())
            parent->setObjectName(QStringLiteral("lqSensorListWidget"));

        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(parent->sizePolicy().hasHeightForWidth());
        parent->setSizePolicy(sizePolicy);
        parent->setMinimumSize(QSize(330, 110));
        parent->setMaximumSize(QSize(16777215, 16777215));

        panelLayout = new QVBoxLayout(parent);
        panelLayout->setObjectName(QStringLiteral("panelLayout"));

        scrollArea = new QScrollArea(parent);
        scrollArea->setObjectName(QStringLiteral("scrollArea"));
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QStringLiteral("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 200, 110));

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        scrollAreaWidgetContents->setLayout(verticalLayout);

        sensorListLayout = new QVBoxLayout();
        sensorListLayout->setObjectName(QStringLiteral("sensorListLayout"));
        verticalLayout->addLayout(sensorListLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        verticalLayout->addItem(verticalSpacer);

        caption = new QLabel();

        scrollArea->setWidget(scrollAreaWidgetContents);
        panelLayout->addWidget(caption);
        panelLayout->addWidget(scrollArea);

        retranslateUi(parent);

        QMetaObject::connectSlotsByName(parent);
    } // setupUi

    void retranslateUi(QWidget *parent)
    {
        parent->setWindowTitle(QApplication::translate("lqSensorListWidget", "Form", nullptr));
    } // retranslateUi
  };
} // namespace Ui
QT_END_NAMESPACE

//-----------------------------------------------------------------------------
lqSensorListWidget* lqSensorListWidget::Instance = 0;

//-----------------------------------------------------------------------------
lqSensorListWidget* lqSensorListWidget::instance()
{
  return lqSensorListWidget::Instance;
}

//-----------------------------------------------------------------------------
lqSensorListWidget::lqSensorListWidget(QWidget* parent) :
  QWidget(parent),
  ui(new Ui::lqSensorListWidget)
{
  ui->setupUi(this);

  // Only 1 lqSensorListWidget instance can be created.
  Q_ASSERT(lqSensorListWidget::Instance == NULL);
  lqSensorListWidget::Instance = this;

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onSourceAdded(pqPipelineSource*)));
  this->connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(onSourceRemoved(pqPipelineSource*)));
  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    this->onSourceAdded(src);
  }

  // Save background color which will represent the not-selected widget
  // Create the darker selected-widget color
  int r, g, b;
  this->palette().color(QPalette::Background).getRgb(&r, &g, &b);
  colorWidget[0] = r;
  colorWidget[1] = g;
  colorWidget[2] = b;
  colorSelectedWidget[0] = (int) r * 0.8;
  colorSelectedWidget[1] = (int) g * 0.8;
  colorSelectedWidget[2] = (int) b * 0.8;
}

//-----------------------------------------------------------------------------
lqSensorListWidget::~lqSensorListWidget()
{
  if (lqSensorListWidget::Instance == this)
  {
    lqSensorListWidget::Instance = 0;
  }
  delete ui;
}

//-----------------------------------------------------------------------------
lqSensorWidget* lqSensorListWidget::findWidget(pqPipelineSource *src) const
{
  if (IsLidarProxy(src->getProxy()))
  {
    for (lqSensorWidget* widget: this->sensorWidgets)
    {
      if (widget->IsWidgetLidarSource(src))
        return widget;
    }
  }

  if (IsPositionOrientationProxy(src->getProxy()))
  {
    for (lqSensorWidget* widget: this->sensorWidgets)
    {
      if (widget->IsWidgetPositionOrientationSource(src))
        return widget;
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::onSourceAdded(pqPipelineSource* src)
{
  if (IsLidarProxy(src->getProxy()))
  {
    // add a lqSensorReaderWidget to layout
    bool alreadyStream = isInLiveSensorMode();
    bool isReader = IsLidarReaderProxy(src->getProxy());
    lqSensorWidget* sensorWidget = isReader
                                    ? static_cast<lqSensorWidget *>(new lqSensorReaderWidget(this))
                                    : static_cast<lqSensorWidget *>(new lqSensorStreamWidget(this));
    sensorWidget->SetLidarSource(src);
    this->sensorWidgets.push_back(sensorWidget);
    this->ui->sensorListLayout->addWidget(sensorWidget);
    sensorWidget->SetCalibrationFunction(this->CalibrationFunction);
    this->ui->caption->setText(sensorWidget->GetExplanationOnUI());
    this->connect(sensorWidget, SIGNAL(selected(lqSensorWidget*)), SLOT(onSelected(lqSensorWidget*)));

    // Emit lidarStreamModeChanged Signal
    if (!alreadyStream && !isReader)
    {
      emit lidarStreamModeChanged(true);
    }
  }
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::onSourceRemoved(pqPipelineSource *src)
{
  if (IsLidarProxy(src->getProxy()))
  {
    // Remove Source associated Widget
    const auto it = std::remove_if(this->sensorWidgets.begin(), this->sensorWidgets.end(),
      [&](const lqSensorWidget* widget)
        {
          return widget->IsWidgetLidarSource(src);
        }
    );
    if (it == this->sensorWidgets.end())
    {
      vtkGenericWarningMacro("LidarSource removed but unaccounted for in lqSensorListWidget");
      return;
    }
    this->sensorWidgets.erase(it);
    (*it)->onClose();

    // Emit lidarStreamModeChanged Signal
    if(IsLidarStreamProxy(src->getProxy()) && !isInLiveSensorMode())
    {
      emit lidarStreamModeChanged(false);
    }
  }

  // If the removed source is a PosOr Source, we have to delete ot from its widget
  else if (IsPositionOrientationProxy(src->getProxy()))
  {
    lqSensorWidget* sensorWidget = this->findWidget(src);
    if (sensorWidget && sensorWidget->GetPositionOrientationSource())
    {
      sensorWidget->SetPositionOrientationSource(nullptr);
      sensorWidget->onUpdateUI();
    }
  }
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::setCalibrationFunction(std::function<void (pqPipelineSource* &, pqPipelineSource* &)> function)
{
  this->CalibrationFunction = function;
  for (lqSensorWidget* widget: this->sensorWidgets)
  {
    widget->SetCalibrationFunction(this->CalibrationFunction);
  }
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::setPosOrSourceToLidarSourceWidget(pqPipelineSource * lidarSrc, pqPipelineSource * posOrSrc)
{
  lqSensorWidget* widget = this->findWidget(lidarSrc);
  if(widget)
  {
    widget->SetPositionOrientationSource(posOrSrc);
    widget->onUpdateUI();
  }
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorListWidget::getPosOrSourceAssociatedToLidarSource(pqPipelineSource * lidarSrc)
{
  lqSensorWidget* widget = this->findWidget(lidarSrc);
  if(widget)
  {
    return widget->GetPositionOrientationSource();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::setSourceToDisplayToLidarSourceWidget(pqPipelineSource * lidarSrc,
                                                               pqPipelineSource * otherSrc)
{
  lqSensorWidget* widget = this->findWidget(lidarSrc);
  if(widget)
  {
    widget->SetSourceToDisplay(otherSrc);
  }
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorListWidget::getSourceToDisplayToLidarSourceWidget(pqPipelineSource * lidarSrc)
{
  lqSensorWidget* widget = this->findWidget(lidarSrc);
  if(widget)
  {
    return widget->GetSourceToDisplay();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::onSelected(lqSensorWidget * widget)
{
  // Un select all element
  for (unsigned int index = 0; index < this->sensorWidgets.size(); index++)
  {
    lqSensorWidget* sw = this->sensorWidgets[index];
    sw->setStyleSheet(QString("background-color:rgb(%1,%2,%3);")
                      .arg(colorWidget[0]).arg(colorWidget[1]).arg(colorWidget[2]));
  }

  // Select the selected element
  widget->setStyleSheet(QString("background-color:rgb(%1,%2,%3);")
                        .arg(colorSelectedWidget[0]).arg(colorSelectedWidget[1]).arg(colorSelectedWidget[2]));

  if(widget->GetSourceToDisplay())
  {
    pqActiveObjects::instance().setActiveSource(widget->GetSourceToDisplay());
  }
  else
  {
    pqActiveObjects::instance().setActiveSource(widget->GetLidarSource());
  }
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorListWidget::getActiveLidarSource()
{
  pqPipelineSource * activeSource = pqActiveObjects::instance().activeSource();

  for (unsigned int index = 0; index < this->sensorWidgets.size(); index++)
  {
    lqSensorWidget* sw = this->sensorWidgets[index];

    if(sw->IsAttachedToWidget(activeSource))
    {
      return sw->GetLidarSource();
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSensorListWidget::getActiveSourceToDisplay()
{
  pqPipelineSource * activeSource = pqActiveObjects::instance().activeSource();

  for (unsigned int index = 0; index < this->sensorWidgets.size(); index++)
  {
    lqSensorWidget* sw = this->sensorWidgets[index];

    if(sw->IsAttachedToWidget(activeSource))
    {
      return sw->GetSourceToDisplay();
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
bool lqSensorListWidget::hasLidarSource()
{
  return !this->sensorWidgets.empty();
}

//-----------------------------------------------------------------------------
bool lqSensorListWidget::isInLiveSensorMode()
{
  // TODO could cache this variable for less looping
  const auto result = std::find_if(this->sensorWidgets.begin(), this->sensorWidgets.end(),
    [&](const lqSensorWidget* widget)
      {
        return qobject_cast<const lqSensorStreamWidget*>(widget) != nullptr;
      }
  );

  return result != this->sensorWidgets.end();
}

//-----------------------------------------------------------------------------
vtkSMProxy* lqSensorListWidget::getLidar()
{
  return this->getActiveLidarSource() ? this->getActiveLidarSource()->getProxy() : nullptr;
}

vtkSMProxy* lqSensorListWidget::getReader()
{
  auto proxy = this->getLidar();
  return IsLidarReaderProxy(proxy) ? proxy : nullptr ;
}

vtkSMProxy* lqSensorListWidget::getSensor()
{
  auto proxy = this->getLidar();
  return IsLidarStreamProxy(proxy) ? proxy : nullptr ;
}

vtkSMProxy* lqSensorListWidget::getTrailingFrame()
{
  // Find First TrailingFrame filter that is connected to ActiveLidarSource
  auto lidar = this->getActiveLidarSource();
  if (!lidar){ return nullptr; }

  // WIP Rely on the fact that frame is first output // getOutputPort("frame")
  Q_FOREACH(pqPipelineSource* src, lidar->getOutputPort(0)->getConsumers())
  {
    vtkSMProxy* proxy = src->getProxy();
    if( IsProxy<vtkTrailingFrame *>(proxy) )
    {
      return proxy;
    }
  }

  return nullptr;
}

vtkSMProxy* lqSensorListWidget::getPosOrSource()
{
  // Find First PosOr filter that is connected to ActiveLidarSource
  auto lidar = this->getActiveLidarSource();
  if (!lidar){ return nullptr; }

  auto posor = this->getPosOrSourceAssociatedToLidarSource(lidar);
  if (!posor){ return nullptr; }

  return posor->getProxy();
}
