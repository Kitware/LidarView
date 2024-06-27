/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarStreamProxy.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMLidarStreamProxy_h
#define vtkSMLidarStreamProxy_h

#include "lvRemotingServerManagerModule.h" // for export macro

#include "vtkSMLidarProxy.h"

/**
 * @class vtkSMLidarStreamProxy
 * @brief proxy to create lidar readers
 *
 * vtkSMLidarStreamProxy is a proxy which handle vtkLidarStream and
 * subclasses.
 *
 */
class LVREMOTINGSERVERMANAGER_EXPORT vtkSMLidarStreamProxy : public vtkSMLidarProxy
{
public:
  static vtkSMLidarStreamProxy* New();
  vtkTypeMacro(vtkSMLidarStreamProxy, vtkSMLidarProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Calls the `GetNeedsUpdate` method on the underlying LiveSource implementation.
   *
   * This function is intended for use in Python when the client does not have
   * the Qt LiveSource behavior running.
   */
  bool DoesNeedsUpdate();

protected:
  vtkSMLidarStreamProxy();
  ~vtkSMLidarStreamProxy() override;

private:
  vtkSMLidarStreamProxy(const vtkSMLidarStreamProxy&) = delete;
  void operator=(const vtkSMLidarStreamProxy&) = delete;
};

#endif // vtkSMLidarStreamProxy_h
