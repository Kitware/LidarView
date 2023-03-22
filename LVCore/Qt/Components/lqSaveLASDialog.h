#ifndef LQSAVELASDIALOG_H
#define LQSAVELASDIALOG_H

#include "lqComponentsModule.h"

#include <QDialog>

namespace Ui {
class lqSaveLASDialog;
}

class LQCOMPONENTS_EXPORT lqSaveLASDialog : public QDialog
{
  Q_OBJECT

public:
  explicit lqSaveLASDialog(QWidget *parent = nullptr);
  ~lqSaveLASDialog();

  bool WriteSRS();
  bool WriteColor();
  int UTMZone();
  int ExportType();

private:
  Ui::lqSaveLASDialog *ui;
};

#endif // LQSAVELASDIALOG_H
