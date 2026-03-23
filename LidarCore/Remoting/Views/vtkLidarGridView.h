/*=========================================================================

  Program: LidarView
  Module:  vtkLidarGridView.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkLidarGridView
 *
 * vtkLidarGridView extends vtkPVRenderView, and add a 2D grid in the view.
 * and set some LidarView specific configuration.
 */

#ifndef vtkLidarGridView_h
#define vtkLidarGridView_h

#include <vtkNew.h> // for vtkNew
#include <vtkPVRenderView.h>
#include <vtkSmartPointer.h> // needed for iVar

#include "lvRemotingViewsModule.h" // for export macro

class vtkLidarGridActor;

class LVREMOTINGVIEWS_EXPORT vtkLidarGridView : public vtkPVRenderView
{
public:
  static vtkLidarGridView* New();
  vtkTypeMacro(vtkLidarGridView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the vtkLidarGridActor to use for the view.
   */
  virtual void SetLidarGridActor(vtkLidarGridActor*);

protected:
  vtkLidarGridView();
  ~vtkLidarGridView() override = default;

  vtkSmartPointer<vtkLidarGridActor> LidarGridActor;

private:
  vtkLidarGridView(const vtkLidarGridView&) = delete;
  void operator=(const vtkLidarGridView&) = delete;
};

#endif
