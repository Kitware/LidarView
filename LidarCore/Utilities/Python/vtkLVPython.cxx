/*=========================================================================

  Program: LidarView
  Module:  vtkLVPython.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLVPython.h"

#include "vtkLVPythonConfigure.h"

#include <vtkProcessModule.h>
#include <vtkPythonInterpreter.h>
#include <vtkResourceFileLocator.h>

void vtkLVPython::PrependLVModulesPythonPath()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkNew<vtkResourceFileLocator> locator;
  locator->SetLogVerbosity(vtkPythonInterpreter::GetLogVerbosity() + 1);
  const std::vector<std::string> prefixes = {
    LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX
#if defined(__APPLE__)
    // if in an App bundle, the `sitepackages` dir is <app_root>/Contents/Python
    ,
    "Contents/Python"
#endif
    ,
    "."
  };
  std::string path =
    locator->Locate(pm->GetSelfDir().c_str(), prefixes, "lidarviewcore/__init__.py");
  if (!path.empty())
  {
    vtkPythonInterpreter::PrependPythonPath(path.c_str());
  }
}
