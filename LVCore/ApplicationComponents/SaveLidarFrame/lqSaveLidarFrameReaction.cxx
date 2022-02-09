#include "lqSaveLidarFrameReaction.h"

#include <sstream>
#include <chrono>
#include <string>

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMWriterFactory.h>

#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqActiveObjects.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqProxyWidgetDialog.h>
#include <pqPVApplicationCore.h>

#include <QFileDialog>
#include <QProgressDialog>
#include <QDebug>

#include "lqHelper.h"
#include "lqSelectLidarFrameDialog.h"
#include "lqSensorListWidget.h"


//-----------------------------------------------------------------------------
lqSaveLidarFrameReaction::lqSaveLidarFrameReaction(QAction* action,
                                                   const QString& writerName,
                                                   const QString& extention,
                                                   bool displaySettings,
                                                   bool useDirectory,
                                                   bool keepNameFromPcapFile,
                                                   bool fileNameWithFrameNumber)
  :pqReaction(action), UseDirectory(useDirectory), KeepNameFromPcapFile(keepNameFromPcapFile),
    FileNameWithFrameNumber(fileNameWithFrameNumber), DisplaySettings(displaySettings),
    WriterName(writerName), Extension(extention)
{
  auto* core = pqApplicationCore::instance();

  pqServerManagerModel* smmodel = core->getServerManagerModel();
  this->connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onUpdateUI(pqPipelineSource*)));
  this->connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(onUpdateUI(pqPipelineSource*)));
  
  this->onUpdateUI(nullptr);
}

//-----------------------------------------------------------------------------
void lqSaveLidarFrameReaction::onUpdateUI(pqPipelineSource*)
{
  bool hasLidarReader = HasProxy<vtkLidarReader>();
  this->parentAction()->setEnabled(hasLidarReader);
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSaveLidarFrameReaction::getCorrectLidar()
{
  pqPipelineSource* lidar = nullptr;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if(smmodel == nullptr)
  {
    return nullptr;
  }

  // If the current selected source belongs to a known widget,
  // We get the lidar associated to the selected widget
  lqSensorListWidget * listSensor = lqSensorListWidget::instance();
  if(listSensor)
  {
    lidar = listSensor->getActiveLidarSource();
  }

  // Otherwise we get the first Lidar of the pipeline
  if(!lidar)
  {
    foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    {
      if(IsLidarReader(src))
      {
         lidar = src;
         break;
      }
    }
  }
  return lidar;
}

//-----------------------------------------------------------------------------
void lqSaveLidarFrameReaction::onTriggered()
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
    nbFrame = tsv->GetNumberOfElements() ? tsv->GetNumberOfElements()-1 : 0 ;
  }

  lqSelectLidarFrameDialog dialog(nbFrame);
  if (dialog.exec())
  {
    saveFrame(lidar->getProxy(), dialog.StartFrame(), dialog.StopFrame());
  }
}

//-----------------------------------------------------------------------------
bool lqSaveLidarFrameReaction::GetFolderAndBaseNameFromUser(vtkSMProxy * lidar)
{
  this->FolderPath = QString("");
  this->BaseName = QString("Frame_");

  if(this->KeepNameFromPcapFile)
  {
    std::string pcapName = vtkSMPropertyHelper(lidar, "FileName").GetAsString();
    QFileInfo fileInfo = QFile(QString::fromStdString(pcapName));
    this->BaseName = fileInfo.baseName();
  }

  if(this->UseDirectory)
  {
    this->FolderPath = QFileDialog::getExistingDirectory(pqCoreUtilities::mainWidget(),
                                                   tr("Save File:"));
    if (this->FolderPath.isEmpty())
    {
      return false;
    }
  }
  else
  {
    QString saveFileName = QFileDialog::getSaveFileName(pqCoreUtilities::mainWidget(),
                                                        tr("Save File:"),
                                                        this->BaseName,
                                                        this->Extension);
    if (saveFileName.isEmpty())
    {
      return false;
    }

    QFileInfo fileInfo = QFile(saveFileName);
    this->FolderPath =  fileInfo.path();
    this->BaseName = fileInfo.baseName();
  }
  return true;
}


//-----------------------------------------------------------------------------
bool lqSaveLidarFrameReaction::saveFrame(vtkSMProxy * lidar, int start, int stop)
{
  if(!GetFolderAndBaseNameFromUser(lidar))
  {
    return false;
  }

  auto* lidarProxy = vtkSMSourceProxy::SafeDownCast(lidar);


  // Create writer asked by the action
  vtkSmartPointer<vtkSMSourceProxy> writer = CreateWriter(lidarProxy);
  if (!writer)
  {
    qCritical() << "Failed to create writer for: " << lidarProxy;
    return false;
  }

  // 2 Let the user configure it
  if (this->DisplaySettings)
  {
    pqProxyWidgetDialog dialog(writer, pqCoreUtilities::mainWidget());
    dialog.setObjectName("WriterSettingsDialog");
    dialog.setEnableSearchBar(dialog.hasAdvancedProperties());
    dialog.setApplyChangesImmediately(true);
    dialog.setWindowTitle(QString("Configure Writer (%1)").arg(writer->GetXMLLabel()));

    // Check to see if this writer has any properties that can be configured by
    // the user. If it does, display the dialog.
    if (dialog.hasVisibleWidgets())
    {
      dialog.exec();
      if (dialog.result() == QDialog::Rejected)
      {
        // The user pressed Cancel so don't write
        return false;
      }
    }
  }

  // 3 iterate over the frame
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene = mgr->getActiveScene();
  double timestep_bakup = scene->getAnimationTime();

  if (start == -1 && stop == -1)
  {
    QString filename = this->FolderPath + "/" + GenerateFileName(this->BaseName, timestep_bakup);
    vtkSMPropertyHelper(writer, "FileName").Set(filename.toStdString().c_str());
    writer->UpdateVTKObjects();
    writer->UpdatePipeline(timestep_bakup);
  }
  else
  {
    auto* tsv = vtkSMDoubleVectorProperty::SafeDownCast(lidarProxy->GetProperty("TimestepValues"));
    QProgressDialog progress("Saving files...", "Abort", 0, stop-start);
    progress.setWindowModality(Qt::ApplicationModal);
    for (int i = start; i <= stop; ++i)
    {
      double timestep = tsv->GetElement(i);
      scene->setAnimationTime(timestep);
      QString filename = this->FolderPath + "/" + GenerateFileName(this->BaseName, timestep);

      vtkSMPropertyHelper(writer, "FileName").Set(filename.toStdString().c_str());
      writer->UpdateVTKObjects();
      writer->UpdatePipeline(timestep);

      progress.setValue(i - start);
      if (progress.wasCanceled())
          break;
    }
    scene->setAnimationTime(timestep_bakup);
  }
  return true;
}

//-----------------------------------------------------------------------------
QString lqSaveLidarFrameReaction::GenerateFileName(QString baseName, double timestep)
{
  QString timeInfo = QString::number(timestep);
  if(this->FileNameWithFrameNumber)
  {
    timeInfo = " (Frame " + QString::number(GetFrameIndexOfTimestamp(timestep)) + ")";
  }

  QString sFilenameMainString = baseName + timeInfo;

  QString sFilenameWithoutExt = sFilenameMainString;
  if (QFileInfo(QFile(sFilenameWithoutExt + "." + this->Extension)).exists())
  {
    int nSuffix = 1;
    while (true)
    {
      QString sFilenameWithoutExtNew = sFilenameMainString + "_" + QString::number(nSuffix);
      if (!QFileInfo(QFile(sFilenameWithoutExtNew + "." + this->Extension)).exists())
      {
        sFilenameWithoutExt = sFilenameWithoutExtNew;
        break;
      }
      nSuffix++;
    }
  }
  return sFilenameWithoutExt + "." + this->Extension;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkSMSourceProxy> lqSaveLidarFrameReaction::CreateWriter(vtkSMSourceProxy* lidarProxy)
{
  vtkSMWriterFactory* writerFactory = vtkSMProxyManager::GetProxyManager()->GetWriterFactory();
  auto* tmp = vtkSMSourceProxy::SafeDownCast(writerFactory->CreateWriter(this->WriterName.toStdString().c_str(), lidarProxy, 0, true));
  return vtkSmartPointer<vtkSMSourceProxy>(tmp);
}
