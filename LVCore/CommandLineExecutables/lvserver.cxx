/*=========================================================================

Program:   LidarView
Module:    lvserver.cxx

Copyright (c) Kitware, Inc.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This classe is greatly inspired from paraview pvserver

#if LIDARVIEW_USE_PYTHON && PARAVIEW_USE_EXTERNAL_VTK
#include "vtkPVPythonInterpreterPath.h"
#include "vtkPythonInterpreter.h"
#endif
#include "vtkCLIOptions.h"
#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVSessionServer.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#if LIDARVIEW_USE_PYTHON
extern "C"
{
  void vtkPVInitializePythonModules();
}
#endif

#include "ParaView_paraview_plugins.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
#if LIDARVIEW_USE_PYTHON && PARAVIEW_USE_EXTERNAL_VTK
  vtkPVPythonInterpreterPath();
#endif

  vtkInitializationHelper::SetApplicationName("LidarViewServer");

  auto cliApp = vtk::TakeSmartPointer(vtkCLIOptions::New());
  cliApp->SetAllowExtras(false);
  cliApp->SetStopOnUnrecognizedArgument(true);
  cliApp->SetDescription(
    "pvserver: the LidarView server\n"
    "=============================\n"
    "This is the LidarView server executable. This is intended for client-server use-cases "
    "which require the client and server to be on different processes, potentially on "
    "different systems.\n\n"
    "Typically, one connects a LidarView / ParaView client (either a graphical client, or a Python-based "
    "client) to this process to drive the data analysis and visualization pipelines.");

  // Init current process type
  auto status =
    vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_SERVER, cliApp);
  cliApp = nullptr;
  if (!status)
  {
    return vtkInitializationHelper::GetExitCode();
  }

  auto config = vtkRemotingCoreConfiguration::GetInstance();

#if LIDARVIEW_USE_PYTHON
  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();
#endif

  // register static plugins
  ParaView_paraview_plugins_initialize();

  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLs("LidarView");

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkMultiProcessController* controller = pm->GetGlobalController();

  vtkPVSessionServer* session = vtkPVSessionServer::New();
  session->SetMultipleConnection(config->GetMultiClientMode());
  session->SetDisableFurtherConnections(config->GetDisableFurtherConnections());

  int process_id = controller->GetLocalProcessId();
  if (process_id == 0)
  {
    // Report status:
    if (config->GetReverseConnection())
    {
      cout << "Connecting to client (reverse connection requested)..." << endl;
    }
    else
    {
      cout << "Waiting for client..." << endl;
    }
  }
  bool success = false;
  if (session->Connect())
  {
    success = true;
    pm->RegisterSession(session);
    if (controller->GetLocalProcessId() == 0)
    {
      while (pm->GetNetworkAccessManager()->ProcessEvents(0) != -1)
      {
      }
    }
    else
    {
      controller->ProcessRMIs();
    }
    pm->UnRegisterSession(session);
  }

  cout << "Exiting..." << endl;
  session->Delete();
  // Exit application
  vtkInitializationHelper::Finalize();
  return success ? vtkInitializationHelper::GetExitCode() : EXIT_FAILURE;
}
