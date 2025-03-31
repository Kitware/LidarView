/*=========================================================================

  Program:   LidarView
  Module:    lqViewFrameActionsImplementation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqViewFrameActionsImplementation.h"

//-----------------------------------------------------------------------------
lqViewFrameActionsImplementation::lqViewFrameActionsImplementation(QObject* parent)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
QList<pqStandardViewFrameActionsImplementation::ViewType>
lqViewFrameActionsImplementation::availableViewTypes()
{
  std::vector<std::tuple<std::string, std::string>> option_view{ { "LidarGridView", "Render View" },
    { "ComparativeRenderView", "Comparative Render View" },
    { "RenderViewWithEDL", "Eye Dome Lighting Render View" },
    { "SpreadSheetView", "SpreadSheet View" },
    { "XYHistogramChartView", "Histogram View" },
    { "XYChartView", "Line Chart View" },
    { "PythonView", "Python View" } };
  QList<pqStandardViewFrameActionsImplementation::ViewType> views;
  for (const auto& [key, value] : option_view)
  {
    pqStandardViewFrameActionsImplementation::ViewType info;
    info.Name = QString(key.c_str());
    info.Label = QString(value.c_str());
    views.push_back(info);
  }
  return views;
}
