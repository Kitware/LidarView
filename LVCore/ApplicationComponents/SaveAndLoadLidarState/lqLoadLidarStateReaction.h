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
