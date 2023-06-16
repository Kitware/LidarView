#ifndef LQUpdateCalibrationReaction_H
#define LQUpdateCalibrationReaction_H

#include "lvApplicationComponentsModule.h"

#include "lqCalibrationStructs.h"
#include "pqReaction.h"

#include "vtkSmartPointer.h"

class pqPipelineSource;
class vtkSMProxy;
class lqCalibrationDialog;

/**
 * @ingroup Reactions
 * Reaction to update the calibration
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqUpdateCalibrationReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqUpdateCalibrationReaction(QAction* action);

  static void setTransform(vtkSMProxy* proxy, vvCalibration::TransformConfig config);

  static void setNetworkCalibration(vtkSMProxy* proxy,
    vvCalibration::NetworkConfig config,
    bool multiSensors);

  static void setReaderCalibration(vtkSMProxy* proxy,
    vvCalibration::Plugin interpreter,
    QString calibrationFile,
    bool isShowFirstAndLastFrame,
    bool isUseRelativeStartTime,
    double timeOffset);

  static void UpdateCalibration(pqPipelineSource*& lidarSource,
    pqPipelineSource*& posOrSource,
    const lqCalibrationDialog& dialog);

  static void UpdateExistingSource(pqPipelineSource*& lidarSource, pqPipelineSource*& posOrSource);

public Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

  /**
   * Monitor the added/deleted source to enable/disable the QAction
   */
  void onSourceAdded(pqPipelineSource* src);
  void onSourceRemoved(pqPipelineSource* src);

private:
  Q_DISABLE_COPY(lqUpdateCalibrationReaction)
};

#endif // LQUpdateCalibrationReaction_H
