#ifndef LQSENSORLISTWIDGET_H
#define LQSENSORLISTWIDGET_H

#include "lqapplicationcomponents_export.h"
#include <functional>
#include <QWidget>

class lqSensorWidget;
class pqPipelineSource;
class pqServerManagerModelItem;
class vtkSMProxy;

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

    /**
     * @brief getActiveLidarSource
     *        Get the Lidar Source associated to the selected pqPipelineSource
     *        If the selected pqPipelineSource is not associated to a sensorWidget, nullptr is returned.
     *        This allows to get the Lidar associated to the other source (as TrainlingFrame) Selected
     * @return The LidarSource associated to the selected widget
     */
    pqPipelineSource* getActiveLidarSource();

    /**
     * @brief getActiveSourceToDisplay
     *        Get the Source To Display associated to the selected pqPipelineSource
     *        If the selected pqPipelineSource is not associated to a sensorWidget, nullptr is returned.
     *        This allows to get the Source To Display (as trailing frame) associated to lidar source Selected
     * @return The OtherSource associated to the selected widget
     */
    pqPipelineSource* getActiveSourceToDisplay();

    /**
     * @brief hasLidarSource
     *        Return true if any LidarSource has been instantiated.
     */
    bool hasLidarSource();
    /**
     * @brief isInLiveSensorMode
     *        Return true if any live sensor exist, that is our definition for live mode.
     */
    bool isInLiveSensorMode();

    // WIP Intended to replace Applogic getReader() / getSensor() / getLidar()
    vtkSMProxy* getLidar ();
    vtkSMProxy* getReader();
    vtkSMProxy* getSensor();
    vtkSMProxy* getTrailingFrame();
    vtkSMProxy* getPosOrSource();

  protected Q_SLOTS:
    void onSourceAdded(pqPipelineSource* src);
    void onSourceRemoved(pqPipelineSource* src);
    void onSelected(lqSensorWidget * widget);

  Q_SIGNALS:
    void lidarStreamModeChanged(bool); // Emitted when LidarStream are now present / absent.

  private:
    Q_DISABLE_COPY(lqSensorListWidget)

    Ui::lqSensorListWidget *ui;
    static lqSensorListWidget* Instance;

    std::vector<lqSensorWidget*> sensorWidgets;
    std::function<void(pqPipelineSource* &, pqPipelineSource* &)> CalibrationFunction;

    int colorWidget[3];
    int colorSelectedWidget[3];
};

#endif // LQSENSORLISTWIDGET_H
