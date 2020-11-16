#ifndef LQHELPER_H
#define LQHELPER_H

#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vector>

/**
 * @brief IsLidarProxy return true if the proxy is a LidarReader or a LidarStream
 * @param proxy to test
 * @return true if the proxy is a LidarReader or a LidarStream
 */
bool IsLidarProxy(vtkSMProxy * proxy);

/**
 * @brief hasLidarProxy look for a lidar proxy in the pipeline
 * @return true if there is a lidar proxy in the pipeline
 */
bool HasLidarProxy();

/**
 * @brief SearchProxyByName search recursively a proxy in an other one using XMLName and XMLLabel
 * @param base_proxy
 * @param proxyName name of the proxy to search
 * @return the subproxy named 'proxyName', nullptr if nothing found
 */
vtkSMProxy* SearchProxyByName(vtkSMProxy * base_proxy, const std::string & proxyName);

/**
 * @brief SearchProxyByGroupName search recursively a proxy in an other one using the XMLGroup
 * @param base_proxy
 * @param proxyGroupName name of the XML group of the proxy to search
 * @return the subproxy of XML group 'proxyGroupName', nullptr if nothing found
 */
vtkSMProxy* SearchProxyByGroupName(vtkSMProxy * base_proxy, const std::string & proxyGroupName);

/**
 * @brief getLidarsProxy search lidar Proxy in the current pipeline
 * @return vector of all lidars proxys found
 */
std::vector<vtkSMProxy*> GetLidarsProxy();

/**
 * @brief GetPropertyFromProxy search a property in a proxy
 * @param proxy
 * @param propNameToFind name of the property to search
 * @return the property named 'propNameToFind', nullptr if nothing found
 */
vtkSMProperty* GetPropertyFromProxy(vtkSMProxy * proxy, const std::string &propNameToFind);

/**
 * @brief UpdateProperty set the values to the property "propNameToFind" of the proxy
 *        If the property is not found in the proxy, a message is displayed but nothing is done.
 *        This function is useful, if the property type is unknown.
 *        If it is known, you should directly use vtkSMPropertyHelper to set the property
 * @param proxy proxy where to search the property
 * @param propNameToFind name of the property
 * @param values properties values to set
 */
void UpdateProperty(vtkSMProxy * proxy, const std::string & propNameToFind,
                    const std::vector<std::string> & values);

#endif // LQHELPER_H
