/*=========================================================================

  Program: LidarView
  Module:  vtkLidarGridActor.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkLidarGridActor
 *
 * vtkLidarGridActor is an actor for the grid mesurement used in LidarView.
 * It uses vtkGridSource as the poly data source.
 */

#ifndef vtkLidarGridActor_h
#define vtkLidarGridActor_h

#include <vtkNew.h> // needed for vtkNew.
#include <vtkOpenGLActor.h>
#include <vtkSmartPointer.h> // needed for iVar

#include "lvRemotingViewsModule.h" // needed for exports

class vtkGridSource;
class vtkPolyDataMapper;

class LVREMOTINGVIEWS_EXPORT vtkLidarGridActor : public vtkOpenGLActor
{
public:
  static vtkLidarGridActor* New();
  vtkTypeMacro(vtkLidarGridActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set lidar grid properties.
   * @sa vtkGridSource properties
   */
  void SetGridScale(double value);
  void SetGridOrigin(double originX, double originY, double originZ);
  void SetGridNormal(double normalX, double normalY, double normalZ);
  void SetGridNumberOfTicks(int value);
  ///@}

protected:
  vtkLidarGridActor();
  ~vtkLidarGridActor() override = default;

private:
  vtkLidarGridActor(const vtkLidarGridActor&) = delete;
  void operator=(const vtkLidarGridActor&) = delete;

  vtkSmartPointer<vtkGridSource> Grid;
  vtkSmartPointer<vtkPolyDataMapper> Mapper;
};

#endif
