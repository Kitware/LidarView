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

}

//-----------------------------------------------------------------------------
lqDockableSpreadSheetReaction::~lqDockableSpreadSheetReaction()
{
  if (dock)
  {
    this->savePersistanceState();
  }
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::onTriggered()
{
  if (!dock)
  {
    this->createDock();
    this->restorePersistanceState();
  }
  else
  {
    this->savePersistanceState();
    this->destroyDock();
  }
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::createDock()
{
  // save the active view, because creating a new view will also make this new view the active one.
  auto& activeObjexts = pqActiveObjects::instance();
  auto* activeView = activeObjexts.activeView();

  // construct the view
  assert(!this->pqView);
  this->pqView = qobject_cast<pqSpreadSheetView*>
      (pqApplicationCore::instance()->getObjectBuilder()->createView(pqSpreadSheetView::spreadsheetViewType(), pqApplicationCore::instance()->getActiveServer(), true));
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

  // construct and show the dock with the view
  assert(!this->dock);
  this->dock = new QDockWidget("SpreadSheet", this->mainWindow);
  // Set an object name to avoid QMainWindow::saveState to ouput
  // "QMainWindow::saveState(): 'objectName' not set for QDockWidget 0x560e7b36a5f0 'SpreadSheet;"
  // This is because QMainWindow::saveState assume a unique name for each QDockWidget or QToolBar
  // see https://doc.qt.io/qt-5/qmainwindow.html#saveState
  this->dock->setObjectName("lqDockableSpreadSheetReactionDock");
  this->dock->setWidget(this->pqView->widget());
  this->mainWindow->addDockWidget(Qt::RightDockWidgetArea, this->dock);
  this->dock->show();

  auto closeAndUpdateUi = [this]()
  {
    this->savePersistanceState();
    this->destroyDock();
    this->parentAction()->setChecked(false);
  };

  // handle case when the dock is closed via dock the close button
  auto* action = this->dock->toggleViewAction();
  QObject::connect(action, &QAction::triggered, this, closeAndUpdateUi);

  // handle case when the dock is closed via the View menu
  auto* button = this->dock->findChild<QAbstractButton*>("qt_dockwidget_closebutton");
  QObject::connect(button, &QAbstractButton::clicked, this, closeAndUpdateUi);
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::destroyDock()
{
  pqApplicationCore::instance()->getObjectBuilder()->destroy(this->pqView);
  this->pqView = nullptr;

  delete this->dock;
  this->dock = nullptr;
}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::savePersistanceState()
{
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
  int showOnlySelectedElement = vtkSMPropertyHelper(this->pqView->getProxy(), "SelectionOnly").GetAsInt();
  settings->setValue(settingKey_SelectionOnly, showOnlySelectedElement);

}

//-----------------------------------------------------------------------------
void lqDockableSpreadSheetReaction::restorePersistanceState()
{
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
