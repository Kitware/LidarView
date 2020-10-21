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
  void ParseJsonContent(Json::Value contents, std::string ObjectName, std::vector<propertyInfo>& propertyInfo);

  /**
   * @brief SearchProxy search recursively a proxy in an other one
   * @param base_proxy
   * @param proxyName name of the proxy to search
   * @return the subproxy of name proxyname, nullptr if nothing found
   */
  vtkSMProxy* SearchProxy(vtkSMProxy * base_proxy, std::string proxyName);

  /**
   * @brief UpdateProperty set the values to the property "propNameToFind" of the proxy
   *        If the property is not found in the proxy, a message is displayed but nothing is done
   * @param proxy proxy where to search the property
   * @param propNameToFind name of the property
   * @param values properties values to set
   */
  void UpdateProperty(vtkSMProxy * proxy, std::string propNameToFind,
                      std::vector<std::string> values);

};

#endif // LQLOADLIDARSTATEREACTION_H
