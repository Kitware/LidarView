#ifndef LQVIEWFRAMEACTIONS_H
#define LQVIEWFRAMEACTIONS_H

#include <pqStandardViewFrameActionsImplementation.h>

#include "lqapplicationcomponents_export.h"


class LQAPPLICATIONCOMPONENTS_EXPORT lqViewFrameActions : public pqStandardViewFrameActionsImplementation
{
  Q_OBJECT
  Q_INTERFACES(pqViewFrameActionsInterface);

public:
  lqViewFrameActions(QObject* parent = nullptr);
  ~lqViewFrameActions() = default;

  /**
  * This method is called after a frame is assigned to a view. The view may be
  * NULL, indicating the frame has been assigned to an empty view. Frames are
  * never reused (except a frame assigned to an empty view).
  */
  void frameConnected(pqViewFrame* frame, pqView* view) override;

  /**
  * check the XML hints to see if a button with the given name
  * should be added to the view frame
  */
  bool isButtonVisible(const std::string& buttonName, pqView* view) override;
};
#endif // LQVIEWFRAMEACTIONS_H
