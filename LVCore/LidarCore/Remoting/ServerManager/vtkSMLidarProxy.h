/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarProxy.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMLidarProxy_h
#define vtkSMLidarProxy_h

#include "lvRemotingServerManagerModule.h" // for export macro

#include <vtkSMSourceProxy.h>

/**
 * @class vtkSMLidarProxy
 * @brief proxy to create lidar readers
 *
 * vtkSMLidarProxy is a proxy which handle both vtkLidarReader and
 * vtkLidarStream.
 *
 */
class LVREMOTINGSERVERMANAGER_EXPORT vtkSMLidarProxy : public vtkSMSourceProxy
{
public:
  static vtkSMLidarProxy* New();
  vtkTypeMacro(vtkSMLidarProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If theses informations are known/accessible, return a string
   * with `Vendor`-`ModelName`, otherwise return an empty string.
   */
  std::string GetLidarInformation();

protected:
  vtkSMLidarProxy();
  ~vtkSMLidarProxy() override;

private:
  vtkSMLidarProxy(const vtkSMLidarProxy&) = delete;
  void operator=(const vtkSMLidarProxy&) = delete;
};

#endif // vtkSMLidarProxy_h
