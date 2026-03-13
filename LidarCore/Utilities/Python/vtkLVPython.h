/*=========================================================================

  Program: LidarView
  Module:  vtkLVPython.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLVPython_h
#define vtkLVPython_h

#include "lvPythonInitializerModule.h"

class LVPYTHONINITIALIZER_EXPORT vtkLVPython
{
public:
  static void PrependLVModulesPythonPath();

private:
  // Disallow creating an instance of this object
  vtkLVPython() {}
};
#endif
