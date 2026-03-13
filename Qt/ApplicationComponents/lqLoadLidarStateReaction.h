#ifndef LQLOADLIDARSTATEREACTION_H
#define LQLOADLIDARSTATEREACTION_H

#include <QString>

#include "pqReaction.h"

#include "lqChooseLidarDialog.h"
#include "lqLidarStateDialog.h"

#include "lqApplicationComponentsModule.h"

#include <vtkSMProxy.h>
#include <vtk_jsoncpp.h>

class pqPipelineSource;
/**
 * @ingroup Reactions
 * Reaction to record stream data in a pcap file
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqLoadLidarStateReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqLoadLidarStateReaction(QAction* action);

  /**
   * @brief Load from a json file the lidar properties state and apply them
   * @param lidarProxy proxy to write the properties into
   */
  static void LoadLidarState(vtkSMProxy* lidarCurrentProxy);

public Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(lqLoadLidarStateReaction)

  /**
   * @brief ParseJsonContent recursive function to create a std::vector
   *        with all properties information containing is a json value
   * @param contents json value to read
   * @param ObjectName name of the current proxy
   * @param propertyInfo vector to write information (name, value) of the found properties
   */
  static void ParseJsonContent(Json::Value contents,
    std::string ObjectName,
    std::vector<propertyInfo>& propertyInfo);
};

#endif // LQLOADLIDARSTATEREACTION_H
