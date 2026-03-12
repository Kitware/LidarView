/*=========================================================================

  Program: LidarView
  Module:  vtkDummyExternalAPI.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <string>

namespace vtkDummyExternalAPI
{
/**
 * Get the underlying library version.
 */
std::string GetVersion();
};
