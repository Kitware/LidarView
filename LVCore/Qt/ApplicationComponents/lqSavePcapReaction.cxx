/*=========================================================================

  Program: LidarView
  Module:  lqSavePcapReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqSavePcapReaction.h"

#include <cstring>
#include <sstream>

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>

#include <QFileInfo>
#include <QProgressDialog>

#include <pqActiveObjects.h>
#include <pqCoreUtilities.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>

#include "lqSelectLidarFrameDialog.h"
#include "vtkLidarReader.h"
#include "vtkSMLidarReaderProxy.h"

//-----------------------------------------------------------------------------
lqSavePcapReaction::lqSavePcapReaction(QAction* action, bool displaySettings)
  : lqSaveLidarFrameReaction(action, "", "pcap", displaySettings)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(portChanged(pqOutputPort*)), this, SLOT(updateEnableState()));

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void lqSavePcapReaction::updateEnableState()
{
  pqOutputPort* port = pqActiveObjects::instance().activePort();
  bool enableState = false;
  if (port)
  {
    enableState = vtkSMLidarReaderProxy::SafeDownCast(port->getSource()->getProxy()) != nullptr;
  }
  this->parentAction()->setEnabled(enableState);
}

//-----------------------------------------------------------------------------
void lqSavePcapReaction::onTriggered()
{
  pqPipelineSource* lidar = this->getCorrectLidar();

  if (!lidar)
  {
    return;
  }

  int nbFrame = 0;
  auto* tsv =
    vtkSMDoubleVectorProperty::SafeDownCast(lidar->getProxy()->GetProperty("TimestepValues"));
  if (tsv)
  {
    nbFrame = tsv->GetNumberOfElements() ? tsv->GetNumberOfElements() - 1 : 0;
  }

  // Set BaseName and FolderPath
  std::string pcapName = vtkSMPropertyHelper(lidar->getProxy(), "FileName").GetAsString();
  QFileInfo fileInfo(QString::fromStdString(pcapName));
  this->FolderPath = fileInfo.path();
  this->BaseName = fileInfo.baseName();

  lqSelectLidarFrameDialog dialog(
    nbFrame, pqCoreUtilities::mainWidget(), lqSelectLidarFrameDialog::ALL_FRAMES);
  if (dialog.exec())
  {
    if (dialog.frameMode() == lqSelectLidarFrameDialog::FRAME_RANGE)
    {
      std::stringstream ss;
      ss << fileInfo.baseName().toStdString() << " (Frame " << dialog.StartFrame() << " to "
         << dialog.StopFrame() << ")";
      this->BaseName = ss.str().c_str();
    }
    this->saveFrame(lidar->getProxy(), dialog.StartFrame(), dialog.StopFrame());
  }
}

//-----------------------------------------------------------------------------
bool lqSavePcapReaction::saveFrame(vtkSMProxy* lidar, int start, int stop)
{
  if (!this->GetFolderAndBaseNameFromUser())
  {
    return false;
  }

  auto* lidarProxy = vtkSMSourceProxy::SafeDownCast(lidar);

  if (!lidarProxy)
  {
    return false;
  }

  vtkLidarReader* reader = vtkLidarReader::SafeDownCast(lidarProxy->GetClientSideObject());
  if (!reader)
  {
    return false;
  }

  QString filename = this->FolderPath + "/" + this->BaseName + "." + this->Extension;

  reader->SaveFrames(start, stop, filename.toStdString());
  return true;
}
