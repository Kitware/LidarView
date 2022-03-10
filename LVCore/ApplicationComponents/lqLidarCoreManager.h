/*=========================================================================

   Program: LidarView
   Module:  lqLidarCoreManager.h

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
#ifndef LQLIDARCOREMANAGER_H
#define LQLIDARCOREMANAGER_H

#include <QObject>

class vtkLidarReader;

class pqPipelineSource;
class pqServer;
class pqPythonShell;

class vtkSMSourceProxy;

class vtkPolyData;

class QWidget;

#include "lqapplicationcomponents_export.h"

/**
 * @class lqLidarCoreManager
 * @brief Base Class for LidarView-based app singleton manager.
 *
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqLidarCoreManager : public QObject
{

  Q_OBJECT

public:
  lqLidarCoreManager(QObject* parent = nullptr);
  virtual ~lqLidarCoreManager();

  // Return the singleton instance, Derived class should return the same and cast it.
  static lqLidarCoreManager* instance();

  // Python Shell related
  void schedulePythonStartup();
  void setPythonShell(pqPythonShell* pythonShell);
  pqPythonShell* getPythonShell();
  void runPython(const QString& statements);
  void forceShowShell();

  // Main render view Creation
  void createMainRenderView();

  // Convenience methods
  static pqServer* getActiveServer();
  static QWidget*  getMainWindow();

  // WIP Those are common to all ParaView-based Apps, it may change in the future
  static void saveFramesToPCAP(
    vtkSMSourceProxy* proxy, int startFrame, int endFrame, const QString& filename);

  static void saveFramesToLAS(vtkLidarReader* reader, vtkPolyData* position, int startFrame,
    int endFrame, const QString& filename, int positionMode);

public Q_SLOTS:
  // Perform delayed python shell startup
  virtual void pythonStartup();

  void onEnableCrashAnalysis(bool crashAnalysisEnabled);

  void onResetDefaultSettings();

  void onMeasurementGrid(bool gridVisible);

  void onResetCameraLidar();
  void onResetCameraToForwardView();
  void onResetCenterToLidarCenter();

private:
  Q_DISABLE_COPY(lqLidarCoreManager);
  static QPointer<lqLidarCoreManager> Instance;

  class pqInternal;
  pqInternal* Internal;

  pqPythonShell* pythonShell;
};

#endif // LQLIDARCOREMANAGER_H
