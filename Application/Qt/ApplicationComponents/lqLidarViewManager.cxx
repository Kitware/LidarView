/*=========================================================================

   Program: LidarView
   Module:  lqLidarViewManager.cxx

   Copyright (c) Kitware Inc.
   All rights reserved.

   LidarView is a free software; you can redistribute it and/or modify it
   under the terms of the LidarView license.

   See LICENSE for the full LidarView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
lqLidarViewManager::~lqLidarViewManager() {}

//-----------------------------------------------------------------------------
void lqLidarViewManager::SetLidarViewDefaultSettings()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  // Set default background color
  const double background1[3] = { 0.0, 0.0, 0.0 };
  const double background2[3] = { 0.0, 0.0, 0.2 };

  for (unsigned short i = 0; i < 3; i++)
  {
    settings->SetSetting("views.RenderView.Background", i, background1[i]);
    settings->SetSetting("views.RenderView.Background2", i, background2[i]);
  }
  settings->SetSetting("views.RenderView.BackgroundColorMode", 1);
  settings->SetSetting("views.RenderView.UseColorPaletteForBackground", 0);

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
