/*=========================================================================

  Program: LidarView
  Module:  lqCommandLineOptionsBehavior.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqCommandLineOptionsBehavior.h"

#include <vtkRemotingCoreConfiguration.h>
#include <vtkSMViewProxy.h>

#include <pqActiveObjects.h>
#include <pqCollaborationEventPlayer.h>
#include <pqCommandLineOptionsBehavior.h>
#include <pqCoreConfiguration.h>
#include <pqRenderView.h>
#include <pqView.h>

#include <QApplication>

#include <iostream>

//-----------------------------------------------------------------------------
lqCommandLineOptionsBehavior::lqCommandLineOptionsBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(),
    &pqActiveObjects::viewChanged,
    this,
    &lqCommandLineOptionsBehavior::onActiveViewChanged);

  // The process must be started from a "main" connection (otherwise xvfb-run will timeout)
  // that is why can't trigger it directly from onActiveViewChanged.
  QObject::connect(this,
    &lqCommandLineOptionsBehavior::lidarGridViewActive,
    this,
    &lqCommandLineOptionsBehavior::processCommandLineOptions);
}

//-----------------------------------------------------------------------------
void lqCommandLineOptionsBehavior::onActiveViewChanged(pqView* view)
{
  pqRenderView* renderView = dynamic_cast<pqRenderView*>(view);
  if (renderView)
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkLidarGridView" && !signalEmitted)
    {
      signalEmitted = true;
      // Delay signal emission as in paraview
      pqTimer::singleShot(250, this, SLOT(emitLidarGridViewActive()));
    }
  }
}

//-----------------------------------------------------------------------------
void lqCommandLineOptionsBehavior::emitLidarGridViewActive()
{
  Q_EMIT this->lidarGridViewActive();
}

//-----------------------------------------------------------------------------
void lqCommandLineOptionsBehavior::processCommandLineOptions()
{
  // Handle server connection.
  pqCommandLineOptionsBehavior::processServerConnection();

  // Handle plugins to load at startup.
  pqCommandLineOptionsBehavior::processPlugins();

  // Handle data.
  pqCommandLineOptionsBehavior::processData();

  // Handle state file.
  pqCommandLineOptionsBehavior::processState();

  // Handle script.
  pqCommandLineOptionsBehavior::processScript();

  // Process live
  pqCommandLineOptionsBehavior::processLive();

  auto rcConfig = vtkRemotingCoreConfiguration::GetInstance();
  if (rcConfig->GetDisableRegistry())
  {
    // a cout for test playback.
    std::cout << "Process started" << std::endl;
  }

  // Process tests.
  const bool success = pqCommandLineOptionsBehavior::processTests();
  if (pqCoreConfiguration::instance()->exitApplicationWhenTestsDone())
  {
    if (pqCoreConfiguration::instance()->testMaster())
    {
      pqCollaborationEventPlayer::wait(1000);
    }

    // Make sure that the pqApplicationCore::prepareForQuit() method
    // get called
    QApplication::closeAllWindows();
    QApplication::instance()->exit(success ? 0 : 1);
  }
}
