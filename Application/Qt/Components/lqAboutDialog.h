/*=========================================================================

  Program:   LidarView
  Module:    lqAboutDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqAboutDialog_h
#define lqAboutDialog_h

#include "lvComponentsModule.h"
#include <QDialog>

namespace Ui
{
class lqAboutDialog;
}

class pqServer;
class QTreeWidget;

/**
 * lqAboutDialog is the about dialog used by LidarView.
 * It provides information about LidarView and current configuration.
 */
class LVCOMPONENTS_EXPORT lqAboutDialog : public QDialog
{
  Q_OBJECT

public:
  lqAboutDialog(QWidget* Parent);
  ~lqAboutDialog() override;

  /**
   * Format the about dialog content into textual form
   */
  QString formatToText();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  /**
   * Saves about dialog formatted output to a file.
   */
  void saveToFile();

  /**
   * Copy about dialog formatted output to the clipboard.
   */
  void copyToClipboard();

protected:
  void AddInformationPanel();
  void AddClientInformation();
  QString formatToText(QTreeWidget* tree);

private:
  Q_DISABLE_COPY(lqAboutDialog)
  Ui::lqAboutDialog* const Ui;
};

#endif // !lqAboutDialog_h
