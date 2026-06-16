/*=========================================================================

  Program:   LidarView
  Module:   vtkLVUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLVUtilities_h
#define vtkLVUtilities_h

#include <vtkObject.h>

#include <string>

#include "lvCommonCoreModule.h"

class vtkFieldData;
class vtkMatrix4x4;
class vtkTransform;

class LVCOMMONCORE_EXPORT vtkLVUtilities : public vtkObject
{
public:
  static vtkLVUtilities* New();
  vtkTypeMacro(vtkLVUtilities, vtkObject);

  ///@{
  /**
   * Set / Get the matrix 4x4 from the vtkTransform to/from the vtkFieldData with the
   * array name associated.
   */
  static bool SetTransformInFieldData(vtkFieldData* out,
    vtkTransform* transform,
    const std::string& name);
  static bool GetTransformFromFieldData(vtkFieldData* in,
    vtkMatrix4x4* matrix,
    const std::string& name);
  static bool GetTransformFromFieldData(vtkFieldData* in,
    vtkTransform* transform,
    const std::string& name);
  ///@}

protected:
  vtkLVUtilities() = default;
  ~vtkLVUtilities() override = default;

private:
  vtkLVUtilities(const vtkLVUtilities&) = delete;
  void operator=(const vtkLVUtilities&) = delete;
};

#endif
