#ifndef LQSENSORWIDGET_H
#define LQSENSORWIDGET_H

#include "lqapplicationcomponents_export.h"

#include <QWidget>
#include <functional>

class pqPipelineSource;

namespace Ui {
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

  public:
    explicit lqSensorWidget(QWidget *parent = 0);
    ~lqSensorWidget();

    void SetLidarSource(pqPipelineSource* src);
    pqPipelineSource* GetLidarSource() const;

    void SetPositionOrientationSource(pqPipelineSource* src);
    pqPipelineSource* GetPositionOrientationSource() const;

    bool IsWidgetLidarSource(pqPipelineSource* src) const;
    bool IsWidgetPositionOrientationSource(pqPipelineSource *src) const;

    void SetCalibrationFunction(std::function<void(pqPipelineSource* &, pqPipelineSource* &)> function);

  public slots:
    void onUpdateUI();
    void onCalibrate();
    void onClose();

private slots:
    void onToggled(bool checked);

private:
    Q_DISABLE_COPY(lqSensorWidget)
    void deleteSource(pqPipelineSource* src);

    bool IsClosing;
    Ui::lqSensorWidget* UI;
    pqPipelineSource* LidarSource;
    pqPipelineSource* PositionOrientationSource;
    std::function<void(pqPipelineSource* &, pqPipelineSource* &)> CalibrationFunction;
};

#endif // LQSENSORWIDGET_H
