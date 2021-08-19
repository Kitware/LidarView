#ifndef LQSAVELASREACTION_H
#define LQSAVELASREACTION_H

#include <pqReaction.h>

#include <vtkSmartPointer.h>

#include "lqapplicationcomponents_export.h"
#include "lqSaveLidarFrameReaction.h"


class pqPipelineSource;
class vtkSMProxy;

/**
* @ingroup Reactions
* Save the frame generate by a LidarReader. The writer should be choosen at compile time.
* Basic example are `PCDWriter` or `DataSetCSVWriter`.
*
* The reaction provide some basic UI to let the user select the frame to save:
* - current frame
* - all frame
* - frame interval
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqSaveLASReaction : public lqSaveLidarFrameReaction
{
    Q_OBJECT
    typedef lqSaveLidarFrameReaction Superclass;

public:
  lqSaveLASReaction(QAction* action, bool displaySettings = false, bool useDirectory = false,
                    bool keepNameFromPcapFile = false, bool fileNameWithFrameNumber = false);

  bool saveFrame(vtkSMProxy * lidar ,int start = -1, int stop = -1) override;

public slots:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(lqSaveLASReaction)

  bool writeFrame(vtkSMProxy * lidar, std::string filename, int currentFrame, int utmZone,
                  int exportType, bool writeColor, bool writeSrs);
};

#endif // LQSAVELASREACTION_H
