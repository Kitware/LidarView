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

#include "vtkSMLidarProxy.h"

/**
 * @class vtkSMLidarReaderProxy
 * @brief proxy to create lidar readers
 *
 * vtkSMLidarReaderProxy is a proxy which handle vtkLidarReader and
 * subclasses.
 *
 */
class LVREMOTINGSERVERMANAGER_EXPORT vtkSMLidarReaderProxy : public vtkSMLidarProxy
{
public:
  static vtkSMLidarReaderProxy* New();
  vtkTypeMacro(vtkSMLidarReaderProxy, vtkSMLidarProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Calls the `SaveFrames` method on the underlying LidarReader implementation.
   */
  void SaveFrames(unsigned int start, unsigned int end, const std::string& filename);

protected:
  vtkSMLidarReaderProxy();
  ~vtkSMLidarReaderProxy() override;

private:
  vtkSMLidarReaderProxy(const vtkSMLidarReaderProxy&) = delete;
  void operator=(const vtkSMLidarReaderProxy&) = delete;
};

#endif // vtkSMLidarReaderProxy_h
