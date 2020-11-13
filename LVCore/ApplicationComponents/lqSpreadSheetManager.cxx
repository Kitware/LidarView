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

#include "lqSpreadSheetManager.h"

#include <pqObjectBuilder.h>
#include <pqApplicationCore.h>
#include <pqActiveObjects.h>

#include <vtkEventQtSlotConnect.h>
#include "vtkSMProperty.h"
#include <vtkSMPropertyHelper.h>
#include <vtkSMViewProxy.h>

#include <assert.h>

//-----------------------------------------------------------------------------
lqSpreadSheetManager::lqSpreadSheetManager(QObject* parent) : QObject(parent)
{
  this->Settings = pqApplicationCore::instance()->settings();
}

//-----------------------------------------------------------------------------
void lqSpreadSheetManager::setSpreadSheetDockWidget(QDockWidget* dock)
{
  this->spreadSheetDock = dock;
}

//-----------------------------------------------------------------------------
void lqSpreadSheetManager::setMainView(pqRenderView* view)
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
void lqSpreadSheetManager::constructSpreadSheet()
{
  assert(this->SpreadSheetView == nullptr);
  this->SpreadSheetView = qobject_cast<pqSpreadSheetView*>
      (pqApplicationCore::instance()->getObjectBuilder()->createView(pqSpreadSheetView::spreadsheetViewType(), pqApplicationCore::instance()->getActiveServer(), true));
  this->SpreadSheetView->rename("main spreadsheet view");
  assert(this->SpreadSheetView != nullptr);

  QObject::connect(this->SpreadSheetView, SIGNAL(endRender()), this, SLOT(onSpreadSheetEndRender()));
  QObject::connect(this, SIGNAL(saveColumnSelection()), this, SLOT(onSaveColumnSelection()));

  // Connect the "Selection only" property to a slot to save this property information
  vtkEventQtSlotConnect* connector = vtkEventQtSlotConnect::New();
  vtkSMViewProxy* viewModule = this->SpreadSheetView->getViewProxy();
  connector->Connect(viewModule->GetProperty("SelectionOnly"), vtkCommand::ModifiedEvent,
    this, SLOT(onSpreadSheetSelectionOnly()));

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
void lqSpreadSheetManager::destructSpreadSheet()
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
void lqSpreadSheetManager::onToggleSpreadSheet(bool toggled)
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
bool lqSpreadSheetManager::isSpreadSheetOpen()
{
  return this->SpreadSheetView != nullptr;
}

//-----------------------------------------------------------------------------
void lqSpreadSheetManager::conditionnallyHideColumn(const std::string& conditionSrcName,
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
void lqSpreadSheetManager::onSpreadSheetEndRender()
{
  // Every time the spreadsheet has to be rendered:
  // - we hide the column "Points_m_XYZ" because it's too wide.
  // - we check which columns have to be displayed (some of them could have been hidden by the user
  // We do this every time (not only for each new frame)
  // Because if the user opens and closes the spreadsheet on the same frame, it should display the same things.


  conditionnallyHideColumn("Data", "Points_m_XYZ"); // hide dupe of pt coords
  conditionnallyHideColumn("TrailingFrame", "Points_m_XYZ");

  this->restoreShowOnlySelectedElement();
  this->restoreColumnSelection();
}

//-----------------------------------------------------------------------------
void lqSpreadSheetManager::onHeaderDataChanged(Qt::Orientation,
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
void lqSpreadSheetManager::onSaveColumnSelection()
{
  if (!this->SpreadSheetView
      || !this->SpreadSheetView->getViewModel()
      || !this->SpreadSheetView->getViewModel()->activeRepresentation())
  {
    return;
  }

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
  this->Settings->setValue(key.c_str(), hiddenArrays);
}

//-----------------------------------------------------------------------------
void lqSpreadSheetManager::restoreColumnSelection()
{
  std::string objectName = this->getCurrentShownObjectName();
  std::string key = getHiddenArraysKey(objectName);
  QStringList hiddenArrays = this->Settings->value(key.c_str()).toStringList();
  foreach (QString array, hiddenArrays)
  {
    this->conditionnallyHideColumn(objectName, array.toStdString());
  }
}

//-----------------------------------------------------------------------------
void lqSpreadSheetManager::onSpreadSheetSelectionOnly()
{
  // Save if "Show only selected elements" is enabled
  int showOnlySelectedElement = vtkSMPropertyHelper(this->SpreadSheetView->getProxy(), "SelectionOnly").GetAsInt();
  this->Settings->setValue("ShowOnlySelectedElement", showOnlySelectedElement);
}

//-----------------------------------------------------------------------------
void lqSpreadSheetManager::restoreShowOnlySelectedElement()
{
  int showOnlySelectedElement = this->Settings->value("ShowOnlySelectedElement").toInt();
  vtkSMPropertyHelper(this->SpreadSheetView->getProxy(), "SelectionOnly").Set(showOnlySelectedElement);
  this->SpreadSheetView->getProxy()->UpdateProperty("SelectionOnly", true);
}

//-----------------------------------------------------------------------------
std::string lqSpreadSheetManager::getCurrentShownObjectName()
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
