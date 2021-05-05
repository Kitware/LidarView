#ifndef LQSENSORWIDGET_H
#define LQSENSORWIDGET_H

#include "lqapplicationcomponents_export.h"

#include <QWidget>
#include <functional>

class pqPipelineSource;
class pqProxy;

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

    void SetSourceToDisplay(pqPipelineSource* src);
    pqPipelineSource* GetSourceToDisplay() const;

    void SetPositionOrientationSource(pqPipelineSource* src);
    pqPipelineSource* GetPositionOrientationSource() const;

    bool IsWidgetLidarSource(pqPipelineSource* src) const;
    bool IsWidgetPositionOrientationSource(pqPipelineSource *src) const;
    bool IsWidgetSourceToDisplay(pqPipelineSource *src) const;

    bool IsAttachedToWidget(pqPipelineSource *src) const;

    void SetCalibrationFunction(std::function<void(pqPipelineSource* &, pqPipelineSource* &)> function);

    virtual QString GetExplanationOnUI() = 0;

  public slots:
    virtual void onUpdateUI();
    void onCalibrate();
    void onClose();
    void onShowHide();
    void onSaveLidarState();
    void onLoadLidarState();

signals:
    void selected(lqSensorWidget*);

protected:
    Q_DISABLE_COPY(lqSensorWidget)
    void deleteSource(pqProxy* src);

    void focusInEvent(QFocusEvent *event) override;

    pqPipelineSource* LidarSource;
    pqPipelineSource* PositionOrientationSource;
    pqPipelineSource* SourceToDisplay;
    bool IsClosing;
    Ui::lqSensorWidget* UI;
    std::function<void(pqPipelineSource* &, pqPipelineSource* &)> CalibrationFunction;
};

#endif // LQSENSORWIDGET_H
