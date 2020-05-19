#ifndef LQSTREAMRECORDDIALOG_H
#define LQSTREAMRECORDDIALOG_H

#include <QDialog>

namespace Ui {
  class lqStreamRecordDialog;
}

/**
 * @brief Dialog so the user can select the recording and calibration filename
 *
 * @todo save all calibration file in case of multiple lidar. Currently, only
 * the first one will be saved
 * @todo provide a feedback to the user if the selected file override anything
 */
class lqStreamRecordDialog : public QDialog
{
  Q_OBJECT

public:
  explicit lqStreamRecordDialog(QWidget *parent = 0, const QString& filename = QString());
  ~lqStreamRecordDialog();

  QString recordingFile();
  QString calibrationFile();

protected slots:
  void onRecordingFileChange(const QString& str);

  void onModifyPath();

private:
  Ui::lqStreamRecordDialog *ui;
};

#endif // LQSTREAMRECORDDIALOG_H
