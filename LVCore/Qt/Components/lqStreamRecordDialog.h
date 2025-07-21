/*=========================================================================

  Program: LidarView
  Module:  lqStreamRecordDialog.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqStreamRecordDialog_h
#define lqStreamRecordDialog_h

#include "lqComponentsModule.h"

#include <pqDialog.h>

#include <QScopedPointer>

class LQCOMPONENTS_EXPORT lqStreamRecordDialog : public pqDialog
{
  Q_OBJECT
  typedef pqDialog Superclass;

public:
  lqStreamRecordDialog(QWidget* parent = nullptr);
  ~lqStreamRecordDialog() override;

  /**
   * Get the path of the directory where PCAPs should be saved.
   */
  QString getFilepath();

  ///@{
  /**
   * Get if split file is required, if false the split interval returns 0.
   * Split interval is in minutes.
   */
  bool isSplitRecordingEnabled();
  unsigned int getSplitInterval();
  ///@}

private:
  Q_DISABLE_COPY(lqStreamRecordDialog)

  class lqInternals;
  QScopedPointer<lqInternals> Internals;
};

#endif
