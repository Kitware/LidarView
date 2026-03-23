#ifndef LQSENSORWIDGET_H
#define LQSENSORWIDGET_H

#include "lqApplicationComponentsModule.h"

#include <QDoubleSpinBox>
#include <QSlider>
#include <QWidget>
#include <functional>

class pqPipelineSource;
class pqProxy;

namespace Ui
{
class lqSensorWidget;
}

/**
 * @brief The lqSensorWidget enable to handle some function of a Lidar with buttons.
 *
 * This widget hold a proxy of LidarStream and enable the user to start, stop or close
 * the stream.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqSensorWidget : public QWidget
{
  Q_OBJECT

  // method to identify the normal of the plane containing the turn
  enum TRANSFORMVALUE_INDEX
  {
    POS_X = 0,
    POS_Y = 1,
    POS_Z = 2,
    ROT_ROLL = 3,
    ROT_PITCH = 4,
    ROT_YAW = 5,
    TRANSFORM_SIZE = 6
  };

public:
  explicit lqSensorWidget(QWidget* parent = 0);
  ~lqSensorWidget();

  void SetLidarSource(pqPipelineSource* src);
  pqPipelineSource* GetLidarSource() const;

  void SetSourceToDisplay(pqPipelineSource* src);
  pqPipelineSource* GetSourceToDisplay() const;

  void SetPositionOrientationSource(pqPipelineSource* src);
  pqPipelineSource* GetPositionOrientationSource() const;

  bool IsWidgetLidarSource(pqPipelineSource* src) const;
  bool IsWidgetPositionOrientationSource(pqPipelineSource* src) const;
  bool IsWidgetSourceToDisplay(pqPipelineSource* src) const;

  bool IsAttachedToWidget(pqPipelineSource* src) const;

  void SetCalibrationFunction(std::function<void(pqPipelineSource*&, pqPipelineSource*&)> function);

  virtual QString GetExplanationOnUI() = 0;

  /*!
   * @brief Function used to get the transfrom data from a proxy and update the internal data /
   * spinbox and sliders of this class accordingly
   */
  void ReadValueFromProxy();

public Q_SLOTS:
  virtual void onUpdateUI();
  void onCalibrate();
  void onClose();
  void onShowHide();
  void onSaveLidarState();
  void onLoadLidarState();

protected Q_SLOTS:
  /*!
   * @brief Function slot used to update the transform of the lidarsource based on the data
   * entered in the UI
   */
  void onUpdateTransform();
  /*!
   * @brief Function slot used to react to an update of a spinbox of a value
   * This will store the data entered in internal variables, update the associated slider
   * accordingly and update the transform
   */
  void onValueSpinBoxUpdate(unsigned int idxValue);
  /*!
   * @brief Function slot used to react to an update of a slider of a value. This will update the
   * internal variables for this and update the transform
   */
  void onSliderUpdate(unsigned int idxValue);
  /*!
   * @brief Function slot used to react of an update of the min / max spinboxes for translations
   * or rotations This will saturate the values used in the transform and update spinboxes /
   * sliders accordingly
   */
  void onMinMaxValueSpinBoxUpdate(unsigned int idxValue);
  /*!
   * @brief Function slot used to react on the toggling of the check box to enable or not the live
   * transform This will hide / show the whole interface and read the data from the proxy to feed
   * to the interface
   */
  void onEnableLiveTransformToggle();

Q_SIGNALS:
  void selected(lqSensorWidget*);
  void buttonDownClicked(lqSensorWidget*);
  void buttonUpClicked(lqSensorWidget*);

protected:
  Q_DISABLE_COPY(lqSensorWidget)
  void deleteSource(pqProxy* src);

  void focusInEvent(QFocusEvent* event) override;

  pqPipelineSource* LidarSource;
  pqPipelineSource* PositionOrientationSource;
  pqPipelineSource* SourceToDisplay;
  bool IsClosing;
  Ui::lqSensorWidget* UI;
  std::function<void(pqPipelineSource*&, pqPipelineSource*&)> CalibrationFunction;
  /*!
   * @brief Function used to trigger the widget to go down in the sensor list
   */
  void onButtonDownClicked();
  /*!
   * @brief Function used to trigger the widget to go up in the sensor list
   */
  void onButtonUpClicked();

  /*!
   * @brief common function to update internal QT widgets to internal values
   */
  void UpdateSpinBoxAndSliderFromInternalValues(unsigned int idxValue);

  // maximum value for the sliders scaling
  double min_Transform[TRANSFORMVALUE_INDEX::TRANSFORM_SIZE]; // in order : X,Y,Z,Roll,Pitch,Yaw

  // minimum value for the slider / spinbox scaling
  double max_Transform[TRANSFORMVALUE_INDEX::TRANSFORM_SIZE]; // in order : X,Y,Z,Roll,Pitch,Yaw

  // current value for each transform
  double curr_Transform[TRANSFORMVALUE_INDEX::TRANSFORM_SIZE]; // in order : X,Y,Z,Roll,Pitch,Yaw

  // current value for each transform
  QSlider* slider_Array[TRANSFORMVALUE_INDEX::TRANSFORM_SIZE]; // in order : X,Y,Z,Roll,Pitch,Yaw

  // current value for each transform
  QDoubleSpinBox*
    spinbox_Array[TRANSFORMVALUE_INDEX::TRANSFORM_SIZE]; // in order : X,Y,Z,Roll,Pitch,Yaw
};

#endif // LQSENSORWIDGET_H
