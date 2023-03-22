#include "lqSaveLASReaction.h"

#include "lqHelper.h"
#include "lqSaveLASDialog.h"
#include "lqSelectLidarFrameDialog.h"
#include "vtkLASFileWriter.h"

#include <vtkNew.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>

#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqPVApplicationCore.h>

#include <QProgressDialog>
#include <QDebug>

//-----------------------------------------------------------------------------
lqSaveLASReaction::lqSaveLASReaction(QAction* action,
                                     bool displaySettings,
                                     bool useDirectory,
                                     bool keepNameFromPcapFile,
                                     bool fileNameWithFrameNumber)
  :lqSaveLidarFrameReaction(action, "LASFileWriter", "las", displaySettings,
                            useDirectory, keepNameFromPcapFile, fileNameWithFrameNumber)
{
}

//-----------------------------------------------------------------------------
void lqSaveLASReaction::onTriggered()
{
  pqPipelineSource * lidar = this->getCorrectLidar();

  if(!lidar)
  {
    return;
  }

  int nbFrame = 0;
  auto* tsv = vtkSMDoubleVectorProperty::SafeDownCast(lidar->getProxy()->GetProperty("TimestepValues"));
  if (tsv)
  {
    nbFrame = tsv->GetNumberOfElements();
  }

  lqSelectLidarFrameDialog dialog(nbFrame);
  if (dialog.exec())
  {
    saveFrame(lidar->getProxy(), dialog.StartFrame(), dialog.StopFrame());
  }
}

//-----------------------------------------------------------------------------
bool lqSaveLASReaction::writeFrame(vtkSMProxy * lidar, std::string filename,
                                   int currentFrame, int utmZone, int exportType,
                                   bool writeColor, bool writeSrs)
{
  vtkLidarReader * lidarReader = vtkLidarReader::SafeDownCast(lidar->GetClientSideObject());
  if(!lidarReader)
  {
    return false;
  }

  vtkNew<vtkLASFileWriter> writer;
  writer->SetInputConnection(0, lidarReader->GetOutputPort(0));
  writer->SetFileName(filename.c_str());
  writer->SetFirstFrame(currentFrame);
  writer->SetLastFrame(currentFrame);
  writer->SetInOutSignedUTMZone(utmZone);
  writer->SetExportType(exportType);
  writer->SetWriteColor(writeColor);
  writer->SetWriteSRS(writeSrs);
  writer->Write();

  return true;
}

//-----------------------------------------------------------------------------
bool lqSaveLASReaction::saveFrame(vtkSMProxy * lidar, int start, int stop)
{
  if(!GetFolderAndBaseNameFromUser(lidar))
  {
    return false;
  }

  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene = mgr->getActiveScene();
  double timestep_bakup = scene->getAnimationTime();

  // We created a specific dialog for 2 reasons :
  // - The user can not change 'First Frame' and 'Last frame'
  // (putting in advanced is not a good solution because the user can still have access to it)
  // The Las Writer does not work if we tried to save multiple frame from a single writer
  // (This is why we create a new writer for each frames)
  // So the dialog would pop up at every frames.
  lqSaveLASDialog dialog(nullptr);
  if(!dialog.exec())
  {
    return false;
  }

  bool writeSrs = dialog.WriteSRS();
  bool writeColor = dialog.WriteColor();
  int utmZone = dialog.UTMZone();
  int exportType = dialog.ExportType();

  // Save only the current frame
  if (start == -1 && stop == -1)
  {
    // We have to set the first frame and the last frame to the current one
    // Otherwise every frames will be saved in a single file
    int currentFrame = GetFrameIndexOfTimestamp(timestep_bakup);
    QString filename = this->FolderPath + "/" + GenerateFileName(this->BaseName, timestep_bakup);

    writeFrame(lidar, filename.toStdString(), currentFrame, utmZone, exportType,
               writeColor, writeSrs);

  }
  else
  {
    // To keep the same behavior of other export format,
    // We export one file per frame
    // We could export several frames in a single file
    // by using "SetFirstFrame" and "SetLastFrame" functions of the vtkLasWriter
    auto* tsv = vtkSMDoubleVectorProperty::SafeDownCast(lidar->GetProperty("TimestepValues"));

    QProgressDialog progress("Saving files...", "Abort", 0, stop-start);
    progress.setWindowModality(Qt::ApplicationModal);
    for (int i = start; i <= stop; ++i)
    {
      double timestep = tsv->GetElement(i);
      // We can not set the animation time of the scene to "timestep" here
      // We think there is a bug in the vtkLasWriter that makes LV bugs
      QString filename = this->FolderPath + "/" + GenerateFileName(this->BaseName, timestep);

      writeFrame(lidar, filename.toStdString(), i, utmZone, exportType, writeColor, writeSrs);

      progress.setValue(i - start);
      if (progress.wasCanceled())
          break;
    }
  }
  return true;
}



