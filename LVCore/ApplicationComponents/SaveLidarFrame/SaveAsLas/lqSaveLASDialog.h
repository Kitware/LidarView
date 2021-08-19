#ifndef LQSAVELASDIALOG_H
#define LQSAVELASDIALOG_H

#include <QDialog>

namespace Ui {
class lqSaveLASDialog;
}

class lqSaveLASDialog : public QDialog
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
