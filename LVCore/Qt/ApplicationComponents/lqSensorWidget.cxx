#include "lqSensorWidget.h"
#include "lqHelper.h"
#include "ui_lqSensorWidget.h"

#include "lqLoadLidarStateReaction.h"
#include "lqSaveLidarStateReaction.h"

#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqProxy.h>
#include <pqSMAdaptor.h>

#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSourceProxy.h>

#include "vtkLidarStream.h"

#include <cassert>

//-----------------------------------------------------------------------------
lqSensorWidget::lqSensorWidget(QWidget* parent)
  : QWidget(parent)
  , LidarSource(nullptr)
  , PositionOrientationSource(nullptr)
  , IsClosing(false)
  , UI(new Ui::lqSensorWidget)
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

  this->UI->enableLiveDataTransformCheckBox->setVisible(true);

  // set up arrays to point on sliders and spinboxes corresponding to each transform param
  this->slider_Array[TRANSFORMVALUE_INDEX::POS_X] = this->UI->TxSlider;
  this->slider_Array[TRANSFORMVALUE_INDEX::POS_Y] = this->UI->TySlider;
  this->slider_Array[TRANSFORMVALUE_INDEX::POS_Z] = this->UI->TzSlider;
  this->slider_Array[TRANSFORMVALUE_INDEX::ROT_ROLL] = this->UI->RollSlider;
  this->slider_Array[TRANSFORMVALUE_INDEX::ROT_PITCH] = this->UI->PitchSlider;
  this->slider_Array[TRANSFORMVALUE_INDEX::ROT_YAW] = this->UI->YawSlider;

  this->spinbox_Array[TRANSFORMVALUE_INDEX::POS_X] = this->UI->TxSpinBox;
  this->spinbox_Array[TRANSFORMVALUE_INDEX::POS_Y] = this->UI->TySpinBox;
  this->spinbox_Array[TRANSFORMVALUE_INDEX::POS_Z] = this->UI->TzSpinBox;
  this->spinbox_Array[TRANSFORMVALUE_INDEX::ROT_ROLL] = this->UI->RollSpinBox;
  this->spinbox_Array[TRANSFORMVALUE_INDEX::ROT_PITCH] = this->UI->PitchSpinBox;
  this->spinbox_Array[TRANSFORMVALUE_INDEX::ROT_YAW] = this->UI->YawSpinBox;

  // Hide UI that shall not be shown if not in live transform mode
  this->onEnableLiveTransformToggle();

  // init internal values from UI defaults
  for (unsigned int idxValue = 0; idxValue < TRANSFORMVALUE_INDEX::TRANSFORM_SIZE; idxValue++)
  {
    min_Transform[idxValue] = this->spinbox_Array[idxValue]->minimum();
    max_Transform[idxValue] = this->spinbox_Array[idxValue]->maximum();
    curr_Transform[idxValue] = this->spinbox_Array[idxValue]->value();
  }

  // connect checkbox to enabler of live transform mode
  connect(this->UI->enableLiveDataTransformCheckBox,
    &QCheckBox::stateChanged,
    this,
    [this] { lqSensorWidget::onEnableLiveTransformToggle(); });

  // connect up and down buttons to emitter of this signal for the sensorlist class
  connect(this->UI->buttonDown,
    &QPushButton::released,
    this,
    [this] { lqSensorWidget::onButtonDownClicked(); });
  connect(this->UI->buttonUp,
    &QPushButton::released,
    this,
    [this] { lqSensorWidget::onButtonUpClicked(); });

  // Connect Slider movement to transform updating function
  connect(this->UI->TxSlider,
    &QSlider::sliderMoved,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::POS_X); });
  connect(this->UI->TySlider,
    &QSlider::sliderMoved,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::POS_Y); });
  connect(this->UI->TzSlider,
    &QSlider::sliderMoved,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::POS_Z); });
  connect(this->UI->RollSlider,
    &QSlider::sliderMoved,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::ROT_ROLL); });
  connect(this->UI->PitchSlider,
    &QSlider::sliderMoved,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::ROT_PITCH); });
  connect(this->UI->YawSlider,
    &QSlider::sliderMoved,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::ROT_YAW); });

  // connect Slider Release signal to make sur last value of the slider is recorded
  connect(this->UI->TxSlider,
    &QSlider::sliderReleased,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::POS_X); });
  connect(this->UI->TySlider,
    &QSlider::sliderReleased,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::POS_Y); });
  connect(this->UI->TzSlider,
    &QSlider::sliderReleased,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::POS_Z); });
  connect(this->UI->RollSlider,
    &QSlider::sliderReleased,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::ROT_ROLL); });
  connect(this->UI->PitchSlider,
    &QSlider::sliderReleased,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::ROT_PITCH); });
  connect(this->UI->YawSlider,
    &QSlider::sliderReleased,
    this,
    [this] { lqSensorWidget::onSliderUpdate(TRANSFORMVALUE_INDEX::ROT_YAW); });

  // connect value spinboxes to corresponding function
  connect(this->UI->TxSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_X); });
  connect(this->UI->TySpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_Y); });
  connect(this->UI->TzSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_Z); });
  connect(this->UI->RollSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_ROLL); });
  connect(this->UI->PitchSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_PITCH); });
  connect(this->UI->YawSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_YAW); });

  // connect min max spin boxes to corresponding function
  connect(this->UI->MinTranslationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_X); });
  connect(this->UI->MinTranslationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_Y); });
  connect(this->UI->MinTranslationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_Z); });
  connect(this->UI->MinRotationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_ROLL); });
  connect(this->UI->MinRotationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_PITCH); });
  connect(this->UI->MinRotationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_YAW); });

  // connect min max spin boxes to corresponding function
  connect(this->UI->MaxTranslationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_X); });
  connect(this->UI->MaxTranslationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_Y); });
  connect(this->UI->MaxTranslationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::POS_Z); });
  connect(this->UI->MaxRotationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_ROLL); });
  connect(this->UI->MaxRotationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_PITCH); });
  connect(this->UI->MaxRotationValueSpinBox,
    &QDoubleSpinBox::editingFinished,
    this,
    [this] { lqSensorWidget::onMinMaxValueSpinBoxUpdate(TRANSFORMVALUE_INDEX::ROT_YAW); });
}

//-----------------------------------------------------------------------------
void lqSensorWidget::SetCalibrationFunction(
  std::function<void(pqPipelineSource*&, pqPipelineSource*&)> function)
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

  if (src)
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
bool lqSensorWidget::IsWidgetLidarSource(pqPipelineSource* src) const
{
  return this->LidarSource == src;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsWidgetPositionOrientationSource(pqPipelineSource* src) const
{
  return this->PositionOrientationSource == src;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsWidgetSourceToDisplay(pqPipelineSource* src) const
{
  return this->SourceToDisplay == src;
}

//-----------------------------------------------------------------------------
bool lqSensorWidget::IsAttachedToWidget(pqPipelineSource* src) const
{
  return this->IsWidgetLidarSource(src) || this->IsWidgetPositionOrientationSource(src) ||
    this->IsWidgetSourceToDisplay(src);
}

//-----------------------------------------------------------------------------
void lqSensorWidget::onCalibrate()
{
  if (this->CalibrationFunction)
  {
    this->CalibrationFunction(this->LidarSource, this->PositionOrientationSource);
  }
  // update internal transform based on updated proxy transform
  ReadValueFromProxy();
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
    lqSaveLidarStateReaction::SaveLidarState(
      this->LidarSource->getProxy(), this->LidarSource->getSMName());
  }
}

//-----------------------------------------------------------------------------
void lqSensorWidget::deleteSource(pqProxy* src)
{
  if (src)
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    for (pqPipelineSource* consumer : static_cast<pqPipelineSource*>(src)->getAllConsumers())
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
  vtkSMProxy* lidarProxy = this->LidarSource->getProxy();
  vtkSMProxyProperty* proxyProperty =
    vtkSMProxyProperty::SafeDownCast(lidarProxy->GetProperty("PacketInterpreter"));
  QString lidarCalibName = "";
  vtkSMProxy* interpreterProxy = vtkSMPropertyHelper(proxyProperty).GetAsProxy();
  if (interpreterProxy)
  {
    vtkSMProperty* lidarPropCalibName = interpreterProxy->GetProperty("CalibrationFileName");
    QString lidarCalibName =
      QString::fromStdString(vtkSMPropertyHelper(lidarPropCalibName).GetAsString());
  }
  QString lidarName = this->LidarSource->getSMName();
  QString calibrationFileName = "Calibration Filename: " + lidarCalibName;
  this->UI->lidarName->setText(lidarName);
  this->UI->CalibrationFile->setText(calibrationFileName);

  // Update UI with Position Orientation information
  if (this->PositionOrientationSource)
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
  Q_EMIT selected(this);
}

void lqSensorWidget::onButtonDownClicked()
{
  Q_EMIT buttonDownClicked(this);
}

void lqSensorWidget::onButtonUpClicked()
{
  Q_EMIT buttonUpClicked(this);
}

void lqSensorWidget::onEnableLiveTransformToggle()
{
  // Get enable flag for the check box
  bool isLiveTransformVisible = this->UI->enableLiveDataTransformCheckBox->isChecked();

  // set / unset visibility of the tools to move the lidar data
  this->UI->TranslationLabel->setVisible(isLiveTransformVisible);
  this->UI->labelMinMaxTranslation->setVisible(isLiveTransformVisible);
  this->UI->MinTranslationValueSpinBox->setVisible(isLiveTransformVisible);
  this->UI->MaxTranslationValueSpinBox->setVisible(isLiveTransformVisible);
  this->UI->TxSlider->setVisible(isLiveTransformVisible);
  this->UI->TxSpinBox->setVisible(isLiveTransformVisible);
  this->UI->TySlider->setVisible(isLiveTransformVisible);
  this->UI->TySpinBox->setVisible(isLiveTransformVisible);
  this->UI->TzSlider->setVisible(isLiveTransformVisible);
  this->UI->TzSpinBox->setVisible(isLiveTransformVisible);
  this->UI->RotationLabel->setVisible(isLiveTransformVisible);
  this->UI->labelMinMaxRotation->setVisible(isLiveTransformVisible);
  this->UI->MinRotationValueSpinBox->setVisible(isLiveTransformVisible);
  this->UI->MaxRotationValueSpinBox->setVisible(isLiveTransformVisible);
  this->UI->RollSlider->setVisible(isLiveTransformVisible);
  this->UI->RollSpinBox->setVisible(isLiveTransformVisible);
  this->UI->PitchSlider->setVisible(isLiveTransformVisible);
  this->UI->PitchSpinBox->setVisible(isLiveTransformVisible);
  this->UI->YawSlider->setVisible(isLiveTransformVisible);
  this->UI->YawSpinBox->setVisible(isLiveTransformVisible);

  // adjust window min size according to the toggle state
  if (isLiveTransformVisible)
  {
    this->setMinimumHeight(225);
  }
  else
  {
    this->setMinimumHeight(190);
  }
  ReadValueFromProxy();
}

void lqSensorWidget::onSliderUpdate(unsigned int idxValue)
{
  // get value from slider
  // update MinMax values
  if (idxValue <= POS_Z)
  {
    min_Transform[idxValue] = this->UI->MinTranslationValueSpinBox->value();
    max_Transform[idxValue] = this->UI->MaxTranslationValueSpinBox->value();
  }
  else
  {
    min_Transform[idxValue] = this->UI->MinRotationValueSpinBox->value();
    max_Transform[idxValue] = this->UI->MaxRotationValueSpinBox->value();
  }

  // compute value represented by the slider position
  double sliderRange = std::max(
    static_cast<double>(slider_Array[idxValue]->maximum() - slider_Array[idxValue]->minimum()),
    1.0);
  double value =
    (static_cast<double>((slider_Array[idxValue]->value() - slider_Array[idxValue]->minimum())) /
      sliderRange) *
      (max_Transform[idxValue] - min_Transform[idxValue]) +
    min_Transform[idxValue];

  // update internal value
  curr_Transform[idxValue] = value;

  // update corresponding spinbox
  this->spinbox_Array[idxValue]->setValue(value);

  // call update transform
  onUpdateTransform();
}

void lqSensorWidget::onValueSpinBoxUpdate(unsigned int idxValue)
{
  // update internal value
  curr_Transform[idxValue] = this->spinbox_Array[idxValue]->value();
  // update slider position based on scaling
  int sliderPosition = slider_Array[idxValue]->minimum() +
    ((slider_Array[idxValue]->maximum() - slider_Array[idxValue]->minimum()) *
      (curr_Transform[idxValue] - min_Transform[idxValue]) /
      (max_Transform[idxValue] - min_Transform[idxValue]));
  this->slider_Array[idxValue]->setValue(sliderPosition);

  // call update transform
  onUpdateTransform();
}

void lqSensorWidget::onMinMaxValueSpinBoxUpdate(unsigned int idxValue)
{
  // update MinMax values ( assumption is that first values are translation then rotation, in
  // X,Y,Z,Roll,Pitch,Yaw order)
  if (idxValue <= POS_Z)
  {
    min_Transform[idxValue] = this->UI->MinTranslationValueSpinBox->value();
    max_Transform[idxValue] = this->UI->MaxTranslationValueSpinBox->value();
  }
  else
  {
    min_Transform[idxValue] = this->UI->MinRotationValueSpinBox->value();
    max_Transform[idxValue] = this->UI->MaxRotationValueSpinBox->value();
  }
  // Saturate internal values
  if (min_Transform[idxValue] > curr_Transform[idxValue])
  {
    curr_Transform[idxValue] = min_Transform[idxValue];
  }
  if (max_Transform[idxValue] < curr_Transform[idxValue])
  {
    curr_Transform[idxValue] = max_Transform[idxValue];
  }

  // update widgets
  UpdateSpinBoxAndSliderFromInternalValues(idxValue);

  // call update transform
  onUpdateTransform();
}

void lqSensorWidget::UpdateSpinBoxAndSliderFromInternalValues(unsigned int idxValue)
{
  // update spinbox min and max
  this->spinbox_Array[idxValue]->setMinimum(min_Transform[idxValue]);
  this->spinbox_Array[idxValue]->setMaximum(max_Transform[idxValue]);

  // update spinbox / slider position based on scaling
  this->spinbox_Array[idxValue]->setValue(curr_Transform[idxValue]);

  int sliderPosition = slider_Array[idxValue]->minimum() +
    ((slider_Array[idxValue]->maximum() - slider_Array[idxValue]->minimum()) *
      (curr_Transform[idxValue] - min_Transform[idxValue]) /
      (max_Transform[idxValue] - min_Transform[idxValue]));
  this->slider_Array[idxValue]->setValue(sliderPosition);
}

void lqSensorWidget::ReadValueFromProxy()
{
  // Get lidar proxy
  if (this->LidarSource)
  {
    vtkSMProxy* lidarProxy = this->LidarSource->getProxy();

    // names of the lidarreader proxy
    std::string propertyNameTranslation = "Position";
    std::string propertyNameRotation = "Rotation";
    std::string TransformproxyName = "Transform2";

    if (lidarProxy)
    {
      // Get transform proxy
      vtkSMProxy* TransformProxy = SearchProxyByName(lidarProxy, TransformproxyName);
      // Get translation property
      vtkSMProperty* propTranslation =
        GetPropertyFromProxy(TransformProxy, propertyNameTranslation);
      std::vector<double> TranslationVector = vtkSMPropertyHelper(propTranslation).GetDoubleArray();

      // Get Translation parameters
      double Tx = TranslationVector[0];
      double Ty = TranslationVector[1];
      double Tz = TranslationVector[2];

      // Get translation property
      vtkSMProperty* propRotation = GetPropertyFromProxy(TransformProxy, propertyNameRotation);
      std::vector<double> RotationVector = vtkSMPropertyHelper(propRotation).GetDoubleArray();
      // Get Rotation parameters
      double Roll = RotationVector[0];
      double Pitch = RotationVector[1];
      double Yaw = RotationVector[2];

      // Concatenate them in a vector
      double TransformParams[TRANSFORMVALUE_INDEX::TRANSFORM_SIZE] = {
        Tx, Ty, Tz, Roll, Pitch, Yaw
      };

      // update internal values
      // Get current min and max for rotation and translation
      double minMinTranslation = this->UI->MinTranslationValueSpinBox->value();
      double maxMaxTranslation = this->UI->MaxTranslationValueSpinBox->value();
      double minMinRotation = this->UI->MinRotationValueSpinBox->value();
      double maxMaxRotation = this->UI->MaxRotationValueSpinBox->value();

      // Loop over parameters to update the current values with what is found  on the proxy
      for (unsigned int idxValue = 0; idxValue < TRANSFORMVALUE_INDEX::TRANSFORM_SIZE; idxValue++)
      {
        // update current value
        curr_Transform[idxValue] = TransformParams[idxValue];

        // update min and max if necessary
        min_Transform[idxValue] = std::min(min_Transform[idxValue], curr_Transform[idxValue]);
        max_Transform[idxValue] = std::max(max_Transform[idxValue], curr_Transform[idxValue]);
        this->spinbox_Array[idxValue]->setMinimum(min_Transform[idxValue]);
        this->spinbox_Array[idxValue]->setMaximum(max_Transform[idxValue]);

        // collect overall minimum and maximum to be able to update the min and max spinboxes
        if (idxValue <= POS_Z)
        {
          minMinTranslation = std::min(minMinTranslation, min_Transform[idxValue]);
          maxMaxTranslation = std::max(maxMaxTranslation, max_Transform[idxValue]);
        }
        else
        {
          minMinRotation = std::min(minMinRotation, min_Transform[idxValue]);
          maxMaxRotation = std::max(maxMaxRotation, max_Transform[idxValue]);
        }
        // update spinbox and slider
        UpdateSpinBoxAndSliderFromInternalValues(idxValue);

      } // End for loop on transforms

      // set min and max spinboxes with the minimum found
      this->UI->MinTranslationValueSpinBox->setValue(minMinTranslation);
      this->UI->MaxTranslationValueSpinBox->setValue(maxMaxTranslation);
      this->UI->MinRotationValueSpinBox->setValue(minMinRotation);
      this->UI->MaxRotationValueSpinBox->setValue(maxMaxRotation);

    } // end if lidarProxy
  }   // end if LidarSource
}

void lqSensorWidget::onUpdateTransform()
{
  vtkSMProxy* lidarProxy = this->LidarSource->getProxy();

  // names of the lidarreader proxy
  std::string propertyNameTranslation = "Position";
  std::string propertyNameRotation = "Rotation";
  std::string TransformproxyName = "Transform2";

  if (lidarProxy)
  {
    // Recover translation from sliders
    double Tx = curr_Transform[TRANSFORMVALUE_INDEX::POS_X];
    double Ty = curr_Transform[TRANSFORMVALUE_INDEX::POS_Y];
    double Tz = curr_Transform[TRANSFORMVALUE_INDEX::POS_Z];
    std::vector<std::string> TranslationValues = {
      std::to_string(Tx), std::to_string(Ty), std::to_string(Tz)
    };

    // Recover Rotation from sliders
    double Roll = curr_Transform[TRANSFORMVALUE_INDEX::ROT_ROLL];
    double Pitch = curr_Transform[TRANSFORMVALUE_INDEX::ROT_PITCH];
    double Yaw = curr_Transform[TRANSFORMVALUE_INDEX::ROT_YAW];
    std::vector<std::string> RotationValues = {
      std::to_string(Roll), std::to_string(Pitch), std::to_string(Yaw)
    };

    // Get transform proxy
    vtkSMProxy* TransformProxy = SearchProxyByName(lidarProxy, TransformproxyName);

    // update proxy properties
    UpdateProxyProperty(TransformProxy, propertyNameTranslation, TranslationValues);
    UpdateProxyProperty(TransformProxy, propertyNameRotation, RotationValues);

    lidarProxy->UpdateSelfAndAllInputs();
    vtkSMSourceProxy* sourcelidarProxy = vtkSMSourceProxy::SafeDownCast(lidarProxy);
    if (sourcelidarProxy)
    {
      sourcelidarProxy->UpdatePipelineInformation();
    }
    pqApplicationCore::instance()->render();
  }
}
