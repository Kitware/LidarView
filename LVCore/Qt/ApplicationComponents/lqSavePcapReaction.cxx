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

#include <pqPipelineSource.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMSourceProxy.h>

#include <QFileDialog>
#include <QProgressDialog>

#include "lqSelectLidarFrameDialog.h"
#include "vtkLidarReader.h"

//-----------------------------------------------------------------------------
lqSavePcapReaction::lqSavePcapReaction(QAction* action, bool displaySettings)
  : lqSaveLidarFrameReaction(action, "", "pcap", displaySettings)
{
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

  lqSelectLidarFrameDialog dialog(
    nbFrame, pqCoreUtilities::mainWidget(), lqSelectLidarFrameDialog::ALL_FRAMES);
  if (dialog.exec())
  {
    this->saveFrame(lidar->getProxy(), dialog.StartFrame(), dialog.StopFrame());
  }
}

//-----------------------------------------------------------------------------
bool lqSavePcapReaction::saveFrame(vtkSMProxy* lidar, int start, int stop)
{
  if (!this->GetFolderAndBaseNameFromUser(lidar))
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

  reader->Open();
  reader->SaveFrame(start, stop, filename.toStdString().c_str());
  reader->Close();
  return true;
}
