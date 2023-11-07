/*=========================================================================

  Program:   LidarView
  Module:    lqLidarViewManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLidarViewManager.h"

#include <vtkSMSettings.h>

#include <QFileInfo>

#include <array>
#include <string>

namespace
{
void SetSetting(vtkSMSettings* settings, std::string name, double value)
{
  settings->SetSetting(name.c_str(), value);
}
}

//-----------------------------------------------------------------------------
lqLidarViewManager::lqLidarViewManager(QObject* parent /*=nullptr*/)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
void lqLidarViewManager::SetLidarViewDefaultSettings()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  // Set default LUT for lidar intensity/reflectivity
  const std::array<std::string, 2> LUTName = { "intensity", "reflectivity" };
  const double LUTValue[12] = { 0.0, 0.0, 0.0, 1.0, 100.0, 1.0, 1.0, 0.0, 256.0, 1.0, 0.0, 0.0 };

  for (const auto& name : LUTName)
  {
    const std::string settingName = "array_lookup_tables." + name + ".PVLookupTable.RGBPoints";
    for (unsigned short i_val = 0; i_val < 12; i_val++)
    {
      settings->SetSetting(settingName.c_str(), i_val, LUTValue[i_val]);
    }
    ::SetSetting(settings, "array_lookup_tables." + name + ".PVLookupTable.ColorSpace", 1);
  }

  // Set default LUT for dual return lidar scalars
  const std::string drLUTName[2] = { "dual_intensity", "dual_distance" };
  const double drLUTValues[2][12] = {
    { -1.0, 0.5, 0.2, 0.8, 0.0, 0.6, 0.6, 0.6, 1.0, 1.0, 0.9, 0.4 },
    { -1.0, 0.1, 0.5, 0.7, 0.0, 0.9, 0.9, 0.9, 1.0, 0.8, 0.2, 0.3 }
  };
  const std::string drLUTAnnotations[2][6] = { { "-1", "low", "0", "dual", "1", "high" },
    { "-1", "near", "0", "dual", "1", "far" } };

  for (unsigned short i_name = 0; i_name < 2; i_name++)
  {
    const std::string name = drLUTName[i_name];
    const std::string settingRGB = "array_lookup_tables." + name + ".PVLookupTable.RGBPoints";
    const std::string settingAnnot = "array_lookup_tables." + name + ".PVLookupTable.Annotations";
    for (unsigned short i_val = 0; i_val < 12; i_val++)
    {
      settings->SetSetting(settingRGB.c_str(), i_val, drLUTValues[i_name][i_val]);
    }
    for (unsigned short i_annot = 0; i_annot < 6; i_annot++)
    {
      settings->SetSetting(settingAnnot.c_str(), i_annot, drLUTAnnotations[i_name][i_annot]);
    }
    ::SetSetting(settings, "array_lookup_tables." + name + ".PVLookupTable.IndexedLookup", 1);
    ::SetSetting(settings, "array_lookup_tables." + name + ".PVLookupTable.NumberOfTableValues", 3);
  }
}
