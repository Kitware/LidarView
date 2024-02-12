/*=========================================================================

  Program: LidarView
  Module:  vtkSMLidarReaderProxy.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMLidarReaderProxy_h
#define vtkSMLidarReaderProxy_h

#include "lvRemotingServerManagerModule.h" // for export macro

#include <vtkSMSourceProxy.h>

/**
 * @class vtkSMLidarReaderProxy
 * @brief proxy to create lidar readers
 *
 * vtkSMLidarReaderProxy is a proxy which handle vtkLidarReader and
 * subclasses.
 *
 */
class LVREMOTINGSERVERMANAGER_EXPORT vtkSMLidarReaderProxy : public vtkSMSourceProxy
{
public:
  static vtkSMLidarReaderProxy* New();
  vtkTypeMacro(vtkSMLidarReaderProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSMLidarReaderProxy(const vtkSMLidarReaderProxy&) = delete;
  void operator=(const vtkSMLidarReaderProxy&) = delete;

protected:
  vtkSMLidarReaderProxy();
  ~vtkSMLidarReaderProxy();
};

#endif // vtkSMLidarReaderProxy_h
