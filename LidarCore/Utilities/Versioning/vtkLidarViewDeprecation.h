/*=========================================================================

  Program: LidarView
  Module:  vtkLidarViewDeprecation.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLidarViewDeprecation_h
#define vtkLidarViewDeprecation_h

#include "vtkLVVersion.h"
#include "vtkParaViewDeprecation.h"

// APIs deprecated in 5.1.0.
#if defined(LIDARVIEW_HIDE_DEPRECATION_WARNINGS)
#define LIDARVIEW_DEPRECATED_IN_5_1_0(reason)
#elif defined(__VTK_WRAP__)
#define LIDARVIEW_DEPRECATED_IN_5_1_0(reason) [[vtk::deprecated(reason, "5.1.0")]]
#else
#define LIDARVIEW_DEPRECATED_IN_5_1_0(reason) PARAVIEW_DEPRECATION(reason)
#endif

#endif

// VTK-HeaderTest-Exclude: vtkParaViewDeprecation.h
