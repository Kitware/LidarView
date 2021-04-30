#include "lqDockableSpreadSheetReaction.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqSettings.h>
#include <pqSpreadSheetView.h>
#include <pqSpreadSheetViewDecorator.h>
#include <pqSpreadSheetViewWidget.h>

#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMViewProxy.h>

#include <QAbstractButton>
#include <QDockWidget>
#include <QHeaderView>
#include <QMainWindow>

#include <cassert>

using namespace std;

constexpr auto settingKey_HidenColumn = "lqDockableSpreadSheetReaction/HiddenColumnLabels";
constexpr auto settingKey_SelectionOnly = "lqDockableSpreadSheetReaction/SelectionOnly";


//-----------------------------------------------------------------------------
lqDockableSpreadSheetReaction::lqDockableSpreadSheetReaction(QAction *action, QMainWindow *mainWindow)
  :pqReaction(action), mainWindow(mainWindow)
{
  assert(!this->dock);
  this->dock = new QDockWidget("SpreadSheet", this->mainWindow);
  // Set an object name to avoid QMainWindow::saveState to ouput
  // "QMainWindow::saveState(): 'objectName' not set for QDockWidget 0x560e7b36a5f0 'SpreadSheet;"
  // This is because QMainWindow::saveState assume a unique name for each QDockWidget or QToolBar
  // see https://doc.qt.io/qt-5/qmainwindow.html#saveState
  this->dock->setObjectName("lqDockableSpreadSheetReactionDock");
  this->dock->hide();
  this->mainWindow->addDockWidget(Qt::RightDockWidgetArea, this->dock);

  this->connect(this->dock, SIGNAL(visibilityChanged(bool)), SLOT(onDockVisibilityChanged()));
}

//-----------------------------------------------------------------------------
lqDockableSpreadSheetReaction::~lqDockableSpreadSheetReaction()
{
  this->savePersistanceState();
  this->destroySpreadSheet();
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::onTriggered()
{
  bool isVisible = this->dock->isVisible();
  this->dock->setVisible(!isVisible);
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::onDockVisibilityChanged()
{
  // update dock content
  if (this->dock->isVisible())
  {
    this->createSpreadSheet();
    this->restorePersistanceState();
  }
  else
  {
    this->savePersistanceState();
    this->destroySpreadSheet();
  }
  // update action
  this->parentAction()->setChecked(this->dock->isVisible());
}


//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::createSpreadSheet()
{
  // save the active view, because creating a new view will also make this new view the active one.
  auto& activeObjexts = pqActiveObjects::instance();
  auto* activeView = activeObjexts.activeView();

  // construct the view
  assert(!this->pqView);
  this->pqView = qobject_cast<pqSpreadSheetView*>
      (pqApplicationCore::instance()->getObjectBuilder()->createView(pqSpreadSheetView::spreadsheetViewType(), pqApplicationCore::instance()->getActiveServer()));
  this->pqView->rename("main spreadsheet view");
  this->pqView->getProxy()->UpdateVTKObjects();

  // restore the active view
  activeObjexts.setActiveView(activeView);

  // Hide the line numbers in the spreadsheet (they bring no information)
  auto* ssvWidget = this->pqView->widget()->findChild<pqSpreadSheetViewWidget*>();
  if (ssvWidget)
  {
    auto* header = ssvWidget->verticalHeader();
    if (header)
    {
      header->hide();
    }
  }

  // add a decorator
  auto decorator = new pqSpreadSheetViewDecorator(this->pqView);
  decorator->setPrecision(3);
  decorator->setFixedRepresentation(true);

  this->dock->setWidget(this->pqView->widget());
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::destroySpreadSheet()
{
  pqApplicationCore::instance()->getObjectBuilder()->destroy(this->pqView);
  this->pqView = nullptr;
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::savePersistanceState()
{
  if (!this->pqView)
    return;

  auto* settings = pqApplicationCore::instance()->settings();
  auto* vproxy = this->pqView->getViewProxy();

  // 1 Save "HiddenColumnLabels"
  auto* vsvp = vtkSMStringVectorProperty::SafeDownCast(vproxy->GetProperty("HiddenColumnLabels"));
  QStringList hiddencolumn;
  for (unsigned int cc = 0, max = vsvp->GetNumberOfElements(); cc < max; ++cc)
  {
    hiddencolumn.append(vsvp->GetElement(cc));
  }
  settings->setValue(settingKey_HidenColumn, hiddencolumn);

  // 2 Save show "SelectionOnly"
  int showOnlySelectedElement = vtkSMPropertyHelper(vproxy, "SelectionOnly").GetAsInt();
  settings->setValue(settingKey_SelectionOnly, showOnlySelectedElement);

}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::restorePersistanceState()
{
  if (!this->pqView)
    return;

  auto* settings = pqApplicationCore::instance()->settings();
  auto* vproxy = this->pqView->getViewProxy();

  // 1 Restore "HiddenColumnLabels"
  // Get values
  QStringList hiddenColumn = settings->value(settingKey_HidenColumn).toStringList();
  vector<string> hiddenColumnSring {"Points_m_XYZ"};
  foreach(QString str, hiddenColumn)
  {
    hiddenColumnSring.push_back(str.toStdString());
  }
  // Set values
  auto* vsvp = vtkSMStringVectorProperty::SafeDownCast(vproxy->GetProperty("HiddenColumnLabels"));
  vsvp->SetElements(hiddenColumnSring);
  vproxy->UpdateVTKObjects();
  pqView->render();

  // 2 retore "SelectionOnly"
  int showOnlySelectedElement = settings->value(settingKey_SelectionOnly).toInt();
  vtkSMPropertyHelper(vproxy, "SelectionOnly").Set(showOnlySelectedElement);
  vproxy->UpdateProperty("SelectionOnly", true);
}
