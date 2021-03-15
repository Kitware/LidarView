#ifndef LQDOCKABLESPREADSHEETREACTION_H
#define LQDOCKABLESPREADSHEETREACTION_H

#include "pqReaction.h"

#include <QStringList>

#include "lqapplicationcomponents_export.h"

class QDockWidget;
class pqSpreadSheetView;
class QMainWindow;

/**
* @ingroup Reactions
* Reaction to open on the fly a Spreadsheet with in new QDockWidget
*
* The spreadsheet has a number specificities:
* - hide Points_m_XYZ array
* - save the "hidden array" in a persistant way (in the user setting file)
* - save (optional) "Show only selected points" state in a persistant way (in the user setting file)
* - add decorator to spreadsheet
* - hide the line number in the spreadsheet
*
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqDockableSpreadSheetReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqDockableSpreadSheetReaction(QAction* action, QMainWindow* mainWindow);
  ~lqDockableSpreadSheetReaction();

public slots:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(lqDockableSpreadSheetReaction)

  void createDock();
  void destroyDock();

  void savePersistanceState();
  void restorePersistanceState();

  QMainWindow* mainWindow = nullptr;
  QDockWidget* dock = nullptr;
  pqSpreadSheetView* pqView = nullptr;
};
#endif // LQDOCKABLESPREADSHEETREACTION_H
