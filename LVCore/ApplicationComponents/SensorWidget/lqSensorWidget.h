#ifndef LQSENSORWIDGET_H
#define LQSENSORWIDGET_H

#include "lqapplicationcomponents_export.h"

#include <QWidget>

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
    void SetPositionOrientationSource(pqPipelineSource* src);
    bool IsLinkedTo(pqPipelineSource* src) const;
    pqPipelineSource* GetPositionOrientationSource() const;
    void UpdateUI();

  public slots:
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
};

#endif // LQSENSORWIDGET_H
