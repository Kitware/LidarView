#ifndef LQLOADLIDARSTATEREACTION_H
#define LQLOADLIDARSTATEREACTION_H

#include <QString>

#include "pqReaction.h"

#include "lqLidarStateDialog.h"

#include "lqapplicationcomponents_export.h"

#include <vtk_jsoncpp.h>
#include <vtkSMProxy.h>

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

  static void LoadLidarState(vtkSMProxy * lidarCurrentProxy);

  /**
   * @brief UpdateProxyProperty set the values to the property "propNameToFind" of the proxy
   *        If the property is not found in the proxy, a message is displayed but nothing is done.
   *        This function is useful, if the property type is unknown.
   *        If it is known, you should directly use vtkSMPropertyHelper to set the property
   * @param proxy proxy where to search the property
   * @param propNameToFind name of the property
   * @param values properties values to set
   * @return 1 if the property has been well set, 0 otherwise
   */
  static int UpdateProxyProperty(vtkSMProxy * proxy, const std::string & propNameToFind,
                                 const std::vector<std::string> & values);

public slots:
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
  static void ParseJsonContent(Json::Value contents, std::string ObjectName, std::vector<propertyInfo>& propertyInfo);

};

#endif // LQLOADLIDARSTATEREACTION_H
