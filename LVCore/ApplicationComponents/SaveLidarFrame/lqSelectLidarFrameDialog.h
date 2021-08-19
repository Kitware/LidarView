#ifndef LQSELECTLIDARFRAMEDIALOG_H
#define LQSELECTLIDARFRAMEDIALOG_H

#include <QDialog>

enum FrameMode
{
  CURRENT_FRAME = 0,
  ALL_FRAMES,
  FRAME_RANGE
};

namespace Ui {
class lqSelectLidarFrameDialog;
}

class lqSelectLidarFrameDialog : public QDialog
{
  Q_OBJECT

public:
  explicit lqSelectLidarFrameDialog(int nbFrame, QWidget *parent = nullptr);
  ~lqSelectLidarFrameDialog();

  FrameMode frameMode() const;
  int StartFrame() const;
  int StopFrame() const;
  void accept() override;
private:
  Ui::lqSelectLidarFrameDialog *ui;
};

#endif // LQSELECTLIDARFRAMEDIALOG_H
