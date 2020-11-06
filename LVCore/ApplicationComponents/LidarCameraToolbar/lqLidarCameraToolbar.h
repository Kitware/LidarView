#ifndef lqLidarCameraToolbar_h
#define lqLidarCameraToolbar_h

#include "lqapplicationcomponents_export.h"
#include <QToolBar>

/**
* lqLidarCameraToolbar is the toolbar that has icons for resetting camera
* orientations as well as zoom-to-data and zoom-to-box.
* This class is a rewrite of the paraview class "pqCameraToolbar"
* more specific to the use of lidar data
*
* As an example :
* At the "set direction to x" the center of rotation was reset the the center of the data
* In case of lidar data we prefer to let he center at (0, 0, 0) = the center of the lidar
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqLidarCameraToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  lqLidarCameraToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  lqLidarCameraToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private slots:
  void updateEnabledState();

private:
  Q_DISABLE_COPY(lqLidarCameraToolbar)
  void constructor();
  QAction* ZoomToDataAction;
};

#endif // lqLidarCameraToolbar_h
