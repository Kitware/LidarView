/*=========================================================================

   Program: LidarView
   Module:  lqChooseLidarDialog.h

   Copyright (c) Kitware Inc.
   All rights reserved.

   LidarView is a free software; you can redistribute it and/or modify it
   under the terms of the LidarView license.

   See LICENSE for the full LidarView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#ifndef lqChooseLidarDialog_H
#define lqChooseLidarDialog_H

#include "lqComponentsModule.h"

#include <QDialog>

/**
 * @brief Dialog so the user can select the lidar to save from
 */
class LQCOMPONENTS_EXPORT lqChooseLidarDialog : public QDialog
{
  Q_OBJECT

public:
  lqChooseLidarDialog(QWidget* parent, const QStringList& lidarNames);
  virtual ~lqChooseLidarDialog(){};

  Q_INVOKABLE int getSelectedLidarIndex() const;

private:
  class pqInternal;
  pqInternal* Internal;

  Q_DISABLE_COPY(lqChooseLidarDialog)
};

#endif // lqChooseLidarDialog_H
