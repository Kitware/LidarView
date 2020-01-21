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

#include "lvSpreadSheetManager.h"

#include <pqObjectBuilder.h>
#include <pqApplicationCore.h>
#include <pqActiveObjects.h>
#include <pqSettings.h>

#include <assert.h>

//-----------------------------------------------------------------------------
lvSpreadSheetManager::lvSpreadSheetManager(QObject* parent) : QObject(parent)
{
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::setSpreadSheetDockWidget(QDockWidget* dock)
{
  this->spreadSheetDock = dock;
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::setMainView(pqRenderView* view)
{
  this->mainView = view;
}

namespace {
//-----------------------------------------------------------------------------
std::vector<QObject*> findChildrenByClassName(QObject* object, std::string className)
{
  std::vector<QObject*> found;
  for (QObject* child : object->children())
  {
     if (std::string(child->metaObject()->className()) == className)
     {
       found.push_back(child);
     }
  }
  return found;
}
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::constructSpreadSheet()
{
  assert(this->SpreadSheetView == nullptr);
  this->SpreadSheetView = qobject_cast<pqSpreadSheetView*>
      (pqApplicationCore::instance()->getObjectBuilder()->createView(pqSpreadSheetView::spreadsheetViewType(), pqApplicationCore::instance()->getActiveServer(), true));
  this->SpreadSheetView->rename("main spreadsheet view");
  assert(this->SpreadSheetView != nullptr);

  QObject::connect(this->SpreadSheetView, SIGNAL(endRender()), this, SLOT(onSpreadSheetEndRender()));
  QObject::connect(this, SIGNAL(saveColumnSelection()), this, SLOT(onSaveColumnSelection()));

  this->SpreadSheetViewDec = new pqSpreadSheetViewDecorator(this->SpreadSheetView);
  this->SpreadSheetViewDec->setPrecision(3);
  this->SpreadSheetViewDec->setFixedRepresentation(true);

  this->spreadSheetDock->setWidget(this->SpreadSheetView->widget());
  this->SpreadSheetView->getProxy()->UpdateVTKObjects();
  this->spreadSheetDock->setVisible(true);

  // Hacky way to hide the line numbers in the spreadsheet (they bring no information)
  std::vector<QObject*> headers = findChildrenByClassName(
                                    findChildrenByClassName(
                                      this->spreadSheetDock->findChild<QObject *>("Viewport"),
                                      "pqSpreadSheetViewWidget")[0],
                                    "QHeaderView");
  // of the two QHeaderViews, we hide the one wich is "vertical"
  for (size_t i = 0; i < headers.size(); i++)
  {
    if (reinterpret_cast<QWidget*>(headers[i])->height() > reinterpret_cast<QWidget*>(headers[i])->width())
    {
      reinterpret_cast<QWidget*>(headers[i])->hide();
    }
  }

  // allow saving of column selection
  if (this->SpreadSheetView->getViewModel() != nullptr)
  {
    QObject::connect(this->SpreadSheetView->getViewModel(),
                     SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
                     this,
                     SLOT(onHeaderDataChanged(Qt::Orientation, int, int)));
  }
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::destructSpreadSheet()
{
  pqActiveObjects::instance().setActiveView(this->mainView);

  this->spreadSheetDock->setVisible(false);
  this->spreadSheetDock->setWidget(nullptr);

  delete this->SpreadSheetViewDec;
  this->SpreadSheetViewDec = nullptr;

  delete this->SpreadSheetView;
  this->SpreadSheetView = nullptr;
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::onToggleSpreadSheet(bool toggled)
{
  if (toggled)
  {
    this->constructSpreadSheet();
    emit this->spreadSheetEnabled(true);
  }
  else
  {
    this->destructSpreadSheet();
    emit this->spreadSheetEnabled(false);
  }
}

//-----------------------------------------------------------------------------
bool lvSpreadSheetManager::isSpreadSheetOpen()
{
  return this->SpreadSheetView != nullptr;
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::conditionnallyHideColumn(const std::string& conditionSrcName,
                                                    const std::string& columnName)
{
  if (!this->SpreadSheetView
      || !this->SpreadSheetView->getViewModel()
      || !this->SpreadSheetView->getViewModel()->activeRepresentation()
      || this->getCurrentShownObjectName() != conditionSrcName)
  {
    return;
  }

  const int cols = this->SpreadSheetView->getViewModel()->columnCount();
  for (int i = 0; i < cols; i++)
  {
    QVariant colHeader = this->SpreadSheetView->getViewModel()->headerData(i, Qt::Orientation::Horizontal);
    if (colHeader.toString().toStdString() == columnName)
    {
      if (this->SpreadSheetView->getViewModel()->isVisible(i))
      {
        this->SpreadSheetView->getViewModel()->setVisible(i, false);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::onSpreadSheetEndRender()
{
  // endRender may not be the best signal to use because it will be called at
  // each frame whereas we would prefer to update only when the source is
  // changed. However I tried pqSpreadSheetView::showing and
  // pqSpreadSheetView::viewportUpdated but none worked because column names
  // are not yet updated
  if (this->getCurrentShownObjectName() != this->lastShownObjectName)
  {
    conditionnallyHideColumn("Data", "Points_m_XYZ"); // hide dupe of pt coords
    conditionnallyHideColumn("TrailingFrame", "Points_m_XYZ");

    this->restoreColumnSelection();
  }
  this->lastShownObjectName = this->getCurrentShownObjectName();
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::onHeaderDataChanged(Qt::Orientation,
                                               int firstColumn,
                                               int lastColumn)
{
  int columnCount = this->SpreadSheetView->getViewModel()->columnCount();
  if (columnCount > 1 && lastColumn - firstColumn != 1)
  {
    // Detected a forced update of header line, not due to user changing
    // column selection, thus no need to save the column selection.
    return;
  }
  if (lastColumn == columnCount - 1)
  {
    // onHeaderDataChanged is called for each col, so we wait until the last one
    emit this->saveColumnSelection();
  }
}

namespace {
std::string getHiddenArraysKey(const std::string& objectName)
{
  return "SpreadSheetManager/HiddenArrays/" + objectName + "/";
}
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::onSaveColumnSelection()
{
  if (!this->SpreadSheetView
      || !this->SpreadSheetView->getViewModel()
      || !this->SpreadSheetView->getViewModel()->activeRepresentation())
  {
    return;
  }

  pqSettings* settings = pqApplicationCore::instance()->settings();

  const int cols = this->SpreadSheetView->getViewModel()->columnCount();
  QStringList hiddenArrays;
  for (int i = 0; i < cols; i++)
  {
    QVariant colHeader = this->SpreadSheetView->getViewModel()->headerData(i, Qt::Orientation::Horizontal);
    if (!this->SpreadSheetView->getViewModel()->isVisible(i))
    {
      hiddenArrays << colHeader.toString();
    }
  }

  std::string key = getHiddenArraysKey(this->getCurrentShownObjectName());
  settings->setValue(key.c_str(), hiddenArrays);
}

//-----------------------------------------------------------------------------
void lvSpreadSheetManager::restoreColumnSelection()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  std::string objectName = this->getCurrentShownObjectName();
  std::string key = getHiddenArraysKey(objectName);
  QStringList hiddenArrays = settings->value(key.c_str()).toStringList();
  foreach (QString array, hiddenArrays)
  {
    this->conditionnallyHideColumn(objectName, array.toStdString());
  }
}

//-----------------------------------------------------------------------------
std::string lvSpreadSheetManager::getCurrentShownObjectName()
{
  if (this->SpreadSheetView &&
    this->SpreadSheetView->getViewModel() &&
    this->SpreadSheetView->getViewModel()->activeRepresentation() &&
    this->SpreadSheetView->getViewModel()->activeRepresentation()->getInput())
  {
    return this->SpreadSheetView->getViewModel()->activeRepresentation()->getInput()->getSMName().toStdString();
  }
  else
  {
    return "";
  }
}
