#ifndef LQHELPER_H
#define LQHELPER_H

#include <vtkSMProxy.h>

/**
 * @brief IsLidarProxy return true if the proxy is a LidarReader or a LidarStream
 * @param proxy to test
 * @return true if the proxy is a LidarReader or a LidarStream
 */
bool IsLidarProxy(vtkSMProxy * proxy);


#endif // LQHELPER_H
