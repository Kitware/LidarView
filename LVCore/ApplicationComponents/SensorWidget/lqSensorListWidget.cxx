#include "lqSensorListWidget.h"
#include "lqSensorWidget.h"

// QT includes.
#include <QApplication>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QScrollArea>

// ParaView Server Manager includes.
#include <vtkSMProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkPVXMLElement.h>
#include <vtkLidarStream.h>

// ParaView includes.
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqComponentsModule.h"
#include "pqView.h"

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

    void setupUi(QWidget *parent)
    {
        if (parent->objectName().isEmpty())
            parent->setObjectName(QStringLiteral("lqSensorListWidget"));

        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(parent->sizePolicy().hasHeightForWidth());
        parent->setSizePolicy(sizePolicy);
        parent->setMinimumSize(QSize(180, 110));
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

        scrollArea->setWidget(scrollAreaWidgetContents);
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

namespace
{
  bool isIMUStream(pqPipelineSource* src)
  {
    return ( src != nullptr
          && src->getProxy() != nullptr
          && src->getProxy()->GetClientSideObject()->IsA("vtkPositionOrientationStream"));
  }

  bool isLidarStream(pqPipelineSource* src)
  {
    return ( src != nullptr
          && src->getProxy() != nullptr
          && src->getProxy()->GetClientSideObject()->IsA("vtkLidarStream"));
  }
}

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
  lastLidarSource(nullptr),
  ui(new Ui::lqSensorListWidget)
{
  ui->setupUi(this);

  // Only 1 lqSensorListWidget instance can be created.
  Q_ASSERT(lqSensorListWidget::Instance == NULL);
  lqSensorListWidget::Instance = this;

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onSourceAdded(pqPipelineSource*)));
  this->connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(onSourceRemoved(pqPipelineSource*)));
  this->connect(smmodel, SIGNAL(nameChanged(pqServerManagerModelItem*)), SLOT(onNameChanged(pqServerManagerModelItem*)));
  this->connect(smmodel, SIGNAL(dataUpdated(pqPipelineSource*)), SLOT(onDataUpdated(pqPipelineSource*)));

  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    this->onSourceAdded(src);
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
  if (isLidarStream(src))
  {
    for (lqSensorWidget* widget: this->sensorWidgets)
    {
      if (widget->IsLinkedTo(src))
        return widget;
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::onSourceAdded(pqPipelineSource* src)
{
  if (isLidarStream(src))
  {
    // keep last added source
    this->lastLidarSource = src;

    // add a sensorWidget to layout
    lqSensorWidget* sensorWidget = new lqSensorWidget(this);
    sensorWidget->SetLidarSource(src);
    this->sensorWidgets.push_back(sensorWidget);
    this->ui->sensorListLayout->addWidget(sensorWidget);
  }

  if (isIMUStream(src))
  {
    lqSensorWidget* sensorWidget = this->findWidget(this->lastLidarSource);
    if (sensorWidget)
      sensorWidget->SetPositionOrientationSource(src);
  }
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::onSourceRemoved(pqPipelineSource *src)
{
  if (isLidarStream(src))
  {
    lqSensorWidget* sensorWidget = this->findWidget(src);
    if (sensorWidget)
    {
      sensorWidget->onClose();
      std::remove(this->sensorWidgets.begin(), this->sensorWidgets.end(), sensorWidget);
    }
  }
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::onNameChanged(pqServerManagerModelItem *item)
{
  auto* src = dynamic_cast<pqPipelineSource*> (item);
  if (isLidarStream(src))
  {
    lqSensorWidget* sensorWidget = this->findWidget(src);
    if (sensorWidget)
      sensorWidget->SetLidarSource(src);
  }
}

//-----------------------------------------------------------------------------
void lqSensorListWidget::onDataUpdated(pqPipelineSource *src)
{
  if (isLidarStream(src))
  {
    lqSensorWidget* sensorWidget = this->findWidget(src);
    if (sensorWidget)
    {
      sensorWidget->UpdateUI();
    }
  }
}
