#ifndef LQSENSORSTREAMWIDGET_H
#define LQSENSORSTREAMWIDGET_H

#include "lqapplicationcomponents_export.h"
#include "lqSensorWidget.h"

#include <QWidget>
#include <functional>

class pqPipelineSource;

/**
 * @brief The lqSensorWidget enable to handle some function of a Lidar with buttons.
 *
 * This widget hold a proxy of LidarStream and enable the user to start, stop or close
 * the stream.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqSensorStreamWidget : public lqSensorWidget
{
  Q_OBJECT

  public:
    explicit lqSensorStreamWidget(QWidget *parent = 0);

    QString GetExplanationOnUI() override;

  public slots:
    void onUpdateUI() override;

public slots:
    void onToggled(bool checked);

private:
    Q_DISABLE_COPY(lqSensorStreamWidget)

};

#endif // LQSENSORSTREAMWIDGET_H
