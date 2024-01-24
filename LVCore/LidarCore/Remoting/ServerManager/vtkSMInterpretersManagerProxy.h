/*=========================================================================

  Program: LidarView
  Module:  vtkSMInterpretersManagerProxy.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMInterpretersManagerProxy_h
#define vtkSMInterpretersManagerProxy_h

#include "lvRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"

/**
 * @class vtkSMInterpretersManagerProxy
 * @brief proxy to find and create lidar reader / stream proxies
 *
 * vtkSMInterpretersManagerProxy is a proxy that helps with finding and creating
 * lidarview interpreters proxies such as derived vtkLidarReader and vtkLidarStream
 *
 */
class LVREMOTINGSERVERMANAGER_EXPORT vtkSMInterpretersManagerProxy : public vtkSMProxy
{
public:
  enum Mode : unsigned int
  {
    READER = 0,
    STREAM
  };

  enum InterpreterType : unsigned int
  {
    LIDAR = 0,
    LIDAR_AND_POSE
  };

  static vtkSMInterpretersManagerProxy* New();
  vtkTypeMacro(vtkSMInterpretersManagerProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Retrieve names of available interpreters (current plugins loaded in LidarView)
   */
  unsigned int GetNumberOfInterpreters();
  const char* GetInterpreterName(unsigned int idx);
  std::vector<const char*> GetAvailableInterpreters();
  ///@}

  /**
   * Retrieve the lidar proxy name to use for a specific interpreter based on the IsStream property
   * and the withPose (GNSS / GPS) argument.
   */
  const char* GetLidarProxyName(const char* interpreter, unsigned int type = LIDAR);

protected:
  vtkSMInterpretersManagerProxy();
  ~vtkSMInterpretersManagerProxy() override;

private:
  vtkSMInterpretersManagerProxy(const vtkSMInterpretersManagerProxy&) = delete;
  void operator=(const vtkSMInterpretersManagerProxy&) = delete;

  const char* GetLidarProxyName(vtkSMProxy* interpreter,
    const char* propertyReaderName,
    const char* propertyStreamName);
};

#endif
