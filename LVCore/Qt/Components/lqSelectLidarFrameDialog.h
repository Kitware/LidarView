/*=========================================================================

  Program: LidarView
  Module:  lqSelectLidarFrameDialog.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef LQSELECTLIDARFRAMEDIALOG_H
#define LQSELECTLIDARFRAMEDIALOG_H

#include "lqComponentsModule.h"

#include <QDialog>

namespace Ui
{
class lqSelectLidarFrameDialog;
}

class LQCOMPONENTS_EXPORT lqSelectLidarFrameDialog : public QDialog
{
  Q_OBJECT

public:
  enum FrameMode
  {
    CURRENT_FRAME = 0,
    ALL_FRAMES,
    FRAME_RANGE
  };

  explicit lqSelectLidarFrameDialog(int nbFrame,
    QWidget* parent = nullptr,
    FrameMode defaultMode = FrameMode::CURRENT_FRAME);
  ~lqSelectLidarFrameDialog();

  FrameMode frameMode() const;
  int StartFrame() const;
  int StopFrame() const;
  void accept() override;

private:
  Ui::lqSelectLidarFrameDialog* ui;
};

#endif // LQSELECTLIDARFRAMEDIALOG_H
