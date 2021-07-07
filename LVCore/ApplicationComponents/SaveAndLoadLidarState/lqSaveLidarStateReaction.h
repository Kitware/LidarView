#ifndef LQSAVELIDARSTATEREACTION_H
#define LQSAVELIDARSTATEREACTION_H

#include <QString>

#include <pqReaction.h>

#include <vtkSMProperty.h>
#include <vtk_jsoncpp.h>

#include "lqLidarStateDialog.h"

#include "lqapplicationcomponents_export.h"

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

  static void SaveLidarState(vtkSMProxy * lidarProxy);


public slots:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;


private:
  Q_DISABLE_COPY(lqSaveLidarStateReaction)

  /**
   * @brief constructPropertiesInfo
   * @param lidarProxy proxy to extract the properties
   * @param propertiesVector contains all the properties (names, value) of the current proxy and ots sub-proxy.
   */
  static void constructPropertiesInfo(vtkSMProxy * lidarProxy, std::vector<propertyInfo>& propertiesVector);

  /**
   * @brief getValueOfPropAsString get the values of a property
   * @param prop property to extract value
   * @return The std::vector containing the values of the property prop.
   *         If the property is a not an array, only the first element is fill
   */
  static std::vector<std::string> getValueOfPropAsString(vtkSMProperty * prop);

};

#endif // LQSAVELIDARSTATEREACTION_H
