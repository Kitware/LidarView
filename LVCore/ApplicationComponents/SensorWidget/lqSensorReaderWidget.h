#ifndef LQSENSORREADERWIDGET_H
#define LQSENSORREADERWIDGET_H

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
class LQAPPLICATIONCOMPONENTS_EXPORT lqSensorReaderWidget : public lqSensorWidget
{
  Q_OBJECT

  public:
    explicit lqSensorReaderWidget(QWidget *parent = 0);

  QString GetExplanationOnUI() override;

  public slots:
    void onUpdateUI() override;

private:
    Q_DISABLE_COPY(lqSensorReaderWidget)

};

#endif // LQSENSORREADERWIDGET_H
