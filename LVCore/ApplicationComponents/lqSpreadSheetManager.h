// Copyright 2020 Kitware, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __lqSpreadSheetManager_h
#define __lqSpreadSheetManager_h

#include <pqRenderView.h>
#include <pqSettings.h>
#include <pqSpreadSheetView.h>
#include <pqSpreadSheetViewModel.h>
#include <pqSpreadSheetViewDecorator.h>

#include <QDockWidget>

#include "lqapplicationcomponents_export.h"

class LQAPPLICATIONCOMPONENTS_EXPORT lqSpreadSheetManager : public QObject
{
  Q_OBJECT

public:
  lqSpreadSheetManager(QObject* parent);
  void setSpreadSheetDockWidget(QDockWidget* dock);
  void setMainView(pqRenderView* view);
signals:
  void spreadSheetEnabled(bool);
  void saveColumnSelection();

protected slots:
  void onToggleSpreadSheet(bool toggle);
  void onSpreadSheetEndRender();

  // This function received a signal from ParaView's SpreadSheetViewModel
  void onHeaderDataChanged(Qt::Orientation orientation,
                           int firstColumn,
                           int lastColumn);

  // only the list of hidden columns is saved
  void onSaveColumnSelection();

  void onSpreadSheetSelectionOnly();
private:
  QDockWidget* spreadSheetDock = nullptr;
  pqRenderView* mainView = nullptr;

  pqSpreadSheetView* SpreadSheetView = nullptr;
  pqSpreadSheetViewDecorator* SpreadSheetViewDec = nullptr;

  pqSettings* Settings;

  void constructSpreadSheet();
  void destructSpreadSheet();
  bool isSpreadSheetOpen();
  void conditionnallyHideColumn(const std::string& conditionSrcName,
                                const std::string& columnName);
  void restoreColumnSelection();
  void restoreShowOnlySelectedElement();
  std::string getCurrentShownObjectName();
};

#endif // __lqSpreadSheetManager_h
