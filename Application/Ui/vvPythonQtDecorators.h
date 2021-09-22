
#ifndef __vvPythonQtDecorators_h
#define __vvPythonQtDecorators_h

#include <PythonQt.h>
#include <QObject>

#include "pqLidarViewManager.h"
#include "Widgets/vvCropReturnsDialog.h"
#include "Widgets/vvSelectFramesDialog.h"
#include <pqActiveObjects.h>

class vvPythonQtDecorators : public QObject
{
  Q_OBJECT

public:
  vvPythonQtDecorators(QObject* parent = 0)
    : QObject(parent)
  {
    this->registerClassForPythonQt(&pqLidarViewManager::staticMetaObject);
    this->registerClassForPythonQt(&vvCropReturnsDialog::staticMetaObject);
    this->registerClassForPythonQt(&vvSelectFramesDialog::staticMetaObject);
    this->registerClassForPythonQt(&pqActiveObjects::staticMetaObject);
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

  pqActiveObjects* new_pqActiveObjects()
  {
    return pqActiveObjects::instancePtr();
  }

  void static_pqLidarViewManager_saveFramesToPCAP(
    vtkSMSourceProxy* arg0, int arg1, int arg2, const QString& arg3)
  {
    pqLidarViewManager::saveFramesToPCAP(arg0, arg1, arg2, arg3);
  }

  void static_pqLidarViewManager_saveFramesToLAS(vtkLidarReader* arg0, vtkPolyData* arg1,
    int arg2, int arg3, const QString& arg4, int arg5)
  {
    pqLidarViewManager::saveFramesToLAS(arg0, arg1, arg2, arg3, arg4, arg5);
  }
};

#endif
