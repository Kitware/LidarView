/*=========================================================================

   Program: LidarView
   Module:  lqPythonQtLidarView.h

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
#ifndef lqPythonQtLidarView_h
#define lqPythonQtLidarView_h

#include <PythonQt.h>
#include <QObject>

#include "lqCropReturnsDialog.h"
#include "lqSelectFramesDialog.h"
#include "lqLidarViewManager.h"

#include "lvApplicationComponentsModule.h"

// WIP Could thinks about subclassing and rework how manager add it
class LVAPPLICATIONCOMPONENTS_EXPORT lqPythonQtLidarView : public QObject
{
  Q_OBJECT

public:
  lqPythonQtLidarView(QObject* parent = 0)
    : QObject(parent)
  {
    this->registerClassForPythonQt(&lqLidarViewManager::staticMetaObject);

    this->registerClassForPythonQt(&lqCropReturnsDialog::staticMetaObject);
    this->registerClassForPythonQt(&lqSelectFramesDialog::staticMetaObject);
  }

  inline void registerClassForPythonQt(const QMetaObject* metaobject)
  {
    PythonQt::self()->registerClass(metaobject, "paraview");
  }

public Q_SLOTS:
  lqCropReturnsDialog* new_lqCropReturnsDialog(QWidget* arg0)
  {
    return new lqCropReturnsDialog(arg0);
  }

  lqSelectFramesDialog* new_lqSelectFramesDialog(QWidget* arg0)
  {
    return new lqSelectFramesDialog(arg0);
  }
};

#endif // lqPythonQtLidarView_h
