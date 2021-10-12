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

#include "Widgets/vvCropReturnsDialog.h"
#include "Widgets/vvSelectFramesDialog.h"

//WIP Could thinks about subclassing and rework how manager add it
class lqPythonQtLidarView : public QObject
{
  Q_OBJECT

public:
  lqPythonQtLidarView(QObject* parent = 0)
    : QObject(parent)
  {
    this->registerClassForPythonQt(&vvCropReturnsDialog::staticMetaObject);
    this->registerClassForPythonQt(&vvSelectFramesDialog::staticMetaObject);
  }

  inline void registerClassForPythonQt(const QMetaObject* metaobject)
  {
    PythonQt::self()->registerClass(metaobject, "paraview");
  }

public slots:

  vvCropReturnsDialog* new_vvCropReturnsDialog(QWidget* arg0)
  {
    return new vvCropReturnsDialog(arg0);
  }

  vvSelectFramesDialog* new_vvSelectFramesDialog(QWidget* arg0)
  {
    return new vvSelectFramesDialog(arg0);
  }
};

#endif //lqPythonQtLidarView_h
