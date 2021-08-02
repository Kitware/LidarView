#ifndef LQSENSORLISTWIDGET_H
#define LQSENSORLISTWIDGET_H

#include "lqapplicationcomponents_export.h"
#include <functional>
#include <QWidget>

class lqSensorWidget;
class pqPipelineSource;
class pqServerManagerModelItem;

namespace Ui {
  class lqSensorListWidget;
}

/**
 * @brief The lqSensorListWidget enable to visualize a list of the opened LidarStream or LidarReader
 *  in the sensorlist Dock.
 *
 * This widget manage a list of lqSensorWidget according to signals generated when a lidar source is
 * added, removed or renamed.
 * The lqSensorListWidget will not taking into accound previous created sensors.
 * It should be created in the mainWindows
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqSensorListWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit lqSensorListWidget(QWidget* parent = 0);
    ~lqSensorListWidget();

    lqSensorWidget* findWidget(pqPipelineSource* src) const;
    static lqSensorListWidget* instance();
    void setCalibrationFunction(std::function<void(pqPipelineSource* &, pqPipelineSource* &)> function);

    // Setter and Getter for the Position Orientation Source associated to the lidar widget lidarSrc
    void setPosOrSourceToLidarSourceWidget(pqPipelineSource * lidarSrc, pqPipelineSource * posOrSrc);
    pqPipelineSource* getPosOrSourceAssociatedToLidarSource(pqPipelineSource * lidarSrc);

    // Setter and Getter for the SourceToDisplay associated to the lidar widget lidarSrc
    // The SourceToDisplay is the source that will be selected in the pipeline browser when the widget is selected
    void setSourceToDisplayToLidarSourceWidget(pqPipelineSource * lidarSrc, pqPipelineSource * otherSrc);
    pqPipelineSource* getSourceToDisplayToLidarSourceWidget(pqPipelineSource * lidarSrc);

  protected slots:
    void onSourceAdded(pqPipelineSource* src);
    void onSourceRemoved(pqPipelineSource* src);
    void onSelected(lqSensorWidget * widget);

  private:
    Q_DISABLE_COPY(lqSensorListWidget)

    Ui::lqSensorListWidget *ui;
    std::vector<lqSensorWidget*> sensorWidgets;
    static lqSensorListWidget* Instance;
    std::function<void(pqPipelineSource* &, pqPipelineSource* &)> CalibrationFunction;

    int colorWidget[3];
    int colorSelectedWidget[3];
};

#endif // LQSENSORLISTWIDGET_H
