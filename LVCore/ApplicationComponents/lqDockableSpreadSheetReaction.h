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
* Reaction to open a Spreadsheet in QDockWidget
*
* The spreadsheet has a number specificities:
* - hide Points_m_XYZ array
* - save the "hidden array" in a persistant way (in the user setting file)
* - save (optional) "Show only selected points" state in a persistant way (in the user setting file)
* - add decorator to spreadsheet
* - hide the line number in the spreadsheet
*
* The QDockWidget always exist but the spreadsheet is create/destruct on the fly when
* the dock visibility change.
*/
class LQAPPLICATIONCOMPONENTS_EXPORT lqDockableSpreadSheetReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  lqDockableSpreadSheetReaction(QAction* action, QMainWindow* mainWindow);
  ~lqDockableSpreadSheetReaction();

protected slots:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

  void onDockVisibilityChanged();

private:
  Q_DISABLE_COPY(lqDockableSpreadSheetReaction)

  void createSpreadSheet();
  void destroySpreadSheet();

  void savePersistanceState();
  void restorePersistanceState();

  QMainWindow* mainWindow = nullptr;
  QDockWidget* dock = nullptr;
  pqSpreadSheetView* pqView = nullptr;
};
#endif // LQDOCKABLESPREADSHEETREACTION_H
