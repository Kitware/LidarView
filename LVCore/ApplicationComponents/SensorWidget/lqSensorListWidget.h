#ifndef LQSENSORLISTWIDGET_H
#define LQSENSORLISTWIDGET_H

#include "lqapplicationcomponents_export.h"
#include <QWidget>

class lqSensorWidget;
class pqPipelineSource;
class pqServerManagerModelItem;

namespace Ui {
  class lqSensorListWidget;
}

/**
 * @brief The lqSensorListWidget enable to visualize a list of the opened LidarStream in the sensorlist Dock.
 *
 * This widget manage a list of lqSensorWidget according to signals generated when a lidar source is
 * added, removed or renamed.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqSensorListWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit lqSensorListWidget(QWidget* parent = 0);
    ~lqSensorListWidget();

    lqSensorWidget* findWidget(pqPipelineSource* src) const;
    static lqSensorListWidget* instance();

  protected slots:
    void onSourceAdded(pqPipelineSource* src);
    void onSourceRemoved(pqPipelineSource* src);
    void onNameChanged(pqServerManagerModelItem* item);

  private:
    Q_DISABLE_COPY(lqSensorListWidget)

    Ui::lqSensorListWidget *ui;
    pqPipelineSource* lastLidarSource;
    std::vector<lqSensorWidget*> sensorWidgets;
    static lqSensorListWidget* Instance;

};

#endif // LQSENSORLISTWIDGET_H
