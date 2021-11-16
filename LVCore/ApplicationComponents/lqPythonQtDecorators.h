/*=========================================================================

   Program: LidarView
   Module:  lqPythonQtDecorators.h

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
#ifndef lqPythonQtDecorators_h
#define lqPythonQtDecorators_h

#include <PythonQt.h>
#include <QObject>

#include "lqLidarCoreManager.h"
#include <pqActiveObjects.h>

class lqPythonQtDecorators : public QObject
{
  Q_OBJECT

public:
  lqPythonQtDecorators(QObject* parent = 0)
    : QObject(parent)
  {
    this->registerClassForPythonQt(&lqLidarCoreManager::staticMetaObject);
  }

  inline void registerClassForPythonQt(const QMetaObject* metaobject)
  {
    PythonQt::self()->registerClass(metaobject, "paraview");
  }

public slots:

  void static_lqLidarCoreManager_saveFramesToPCAP(
    vtkSMSourceProxy* arg0, int arg1, int arg2, const QString& arg3)
  {
    lqLidarCoreManager::saveFramesToPCAP(arg0, arg1, arg2, arg3);
  }

  void static_lqLidarCoreManager_saveFramesToLAS(vtkLidarReader* arg0, vtkPolyData* arg1,
    int arg2, int arg3, const QString& arg4, int arg5)
  {
    lqLidarCoreManager::saveFramesToLAS(arg0, arg1, arg2, arg3, arg4, arg5);
  }
};

#endif //lqPythonQtDecorators_h
