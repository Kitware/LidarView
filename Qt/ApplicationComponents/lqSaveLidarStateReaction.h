#ifndef LQSAVELIDARSTATEREACTION_H
#define LQSAVELIDARSTATEREACTION_H

#include <QString>

#include <pqReaction.h>

#include <vtkSMProperty.h>
#include <vtk_jsoncpp.h>

#include "lqChooseLidarDialog.h"
#include "lqLidarStateDialog.h"

#include "lqApplicationComponentsModule.h"

class pqPipelineSource;
/**
 * @ingroup Reactions
 * Reaction to record stream data in a pcap file
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqSaveLidarStateReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqSaveLidarStateReaction(QAction* action);

  /**
   * @brief Save to a json file the current lidar properties state
   * @param lidarProxy proxy to extract the properties
   * @param lidarName lidar name that will be used in default filename
   */
  static void SaveLidarState(vtkSMProxy* lidarProxy, const QString& lidarName);

public Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(lqSaveLidarStateReaction)

  /**
   * @brief constructPropertiesInfo
   * @param lidarProxy proxy to extract the properties
   * @param propertiesVector contains all the properties (names, value) of the current proxy and ots
   * sub-proxy.
   */
  static void constructPropertiesInfo(vtkSMProxy* lidarProxy,
    std::vector<propertyInfo>& propertiesVector);

  /**
   * @brief getValueOfPropAsString get the values of a property
   * @param prop property to extract value
   * @return The std::vector containing the values of the property prop.
   *         If the property is a not an array, only the first element is fill
   */
  static std::vector<std::string> getValueOfPropAsString(vtkSMProperty* prop);
};

#endif // LQSAVELIDARSTATEREACTION_H
