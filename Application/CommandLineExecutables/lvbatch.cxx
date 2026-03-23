/*=========================================================================

Program:   LidarView
Module:    lvbatch.cxx

Copyright (c) Kitware, Inc.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "lvpython.h" // include this 1st.

#include "vtkOutputWindow.h"
#include "vtkProcessModule.h"

#if PARAVIEW_USE_PYTHON && PARAVIEW_USE_EXTERNAL_VTK
#include "vtkPVPythonInterpreterPath.h"
#include "vtkPythonInterpreter.h"
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
int wmain(int argc, wchar_t* wargv[])
#else
int main(int argc, char* argv[])
#endif
{
#if defined(_WIN32) && !defined(__MINGW32__)
  vtkWideArgsConverter converter(argc, wargv);
  char** argv = converter.GetArgs();
#endif
#if PARAVIEW_USE_EXTERNAL_VTK
  vtkPVPythonInterpreterPath();
#endif
  // Setup the output window to be vtkOutputWindow, rather than platform
  // specific one. This avoids creating vtkWin32OutputWindow on Windows, for
  // example, which puts all Python errors in a window rather than the terminal
  // as one would expect.
  auto opwindow = vtkOutputWindow::New();
  vtkOutputWindow::SetInstance(opwindow);
  opwindow->Delete();

  return LidarViewPython::Run(vtkProcessModule::PROCESS_BATCH, argc, argv);
}
