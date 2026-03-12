/*=========================================================================

  Program: LidarView
  Module:  lqSaveLidarFrameReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqSaveLidarFrameReaction.h"

#include <chrono>
#include <sstream>
#include <string>

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMWriterFactory.h>

#include <pqActiveObjects.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqPVApplicationCore.h>
#include <pqProxyWidgetDialog.h>

#include <QDebug>
#include <QProgressDialog>

#include "lqHelper.h"
#include "lqSelectLidarFrameDialog.h"
#include "lqSensorListWidget.h"

#include "vtkSMLidarReaderProxy.h"

//-----------------------------------------------------------------------------
lqSaveLidarFrameReaction::lqSaveLidarFrameReaction(QAction* action,
  const QString& writerName,
  const QString& extension,
  bool displaySettings)
  : pqReaction(action)
  , DisplaySettings(displaySettings)
  , WriterName(writerName)
  , Extension(extension)
{
  auto* core = pqApplicationCore::instance();

  pqServerManagerModel* smmodel = core->getServerManagerModel();
  this->connect(
    smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onUpdateUI(pqPipelineSource*)));
  this->connect(
    smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(onUpdateUI(pqPipelineSource*)));

  this->onUpdateUI(nullptr);
}

//-----------------------------------------------------------------------------
void lqSaveLidarFrameReaction::onUpdateUI(pqPipelineSource* source)
{
  bool hasLidarReader = HasProxy<vtkLidarReader>();

  bool hasWriter = false;
  if (source)
  {
    vtkSMSourceProxy* proxy = source->getSourceProxy();
    vtkSMWriterFactory* writerFactory = vtkSMProxyManager::GetProxyManager()->GetWriterFactory();
    std::string supportedWriters = writerFactory->GetSupportedWriterProxies(proxy, 0);
    hasWriter = supportedWriters.find(this->WriterName.toStdString()) != std::string::npos;
  }

  this->parentAction()->setEnabled(hasLidarReader && hasWriter);
}

//-----------------------------------------------------------------------------
pqPipelineSource* lqSaveLidarFrameReaction::getCorrectLidar()
{
  pqPipelineSource* lidar = nullptr;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  if (smmodel == nullptr)
  {
    return nullptr;
  }

  // If the current selected source belongs to a known widget,
  // We get the lidar associated to the selected widget
  lqSensorListWidget* listSensor = lqSensorListWidget::instance();
  if (listSensor)
  {
    lidar = listSensor->getActiveLidarSource();
  }

  // Otherwise we get the first Lidar of the pipeline
  if (!lidar)
  {
    Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    {
      bool isReader = vtkSMLidarReaderProxy::SafeDownCast(src->getSourceProxy()) != nullptr;
      if (isReader)
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

  lqSelectLidarFrameDialog dialog(nbFrame);
  if (dialog.exec())
  {
    this->saveFrame(lidar->getProxy(), dialog.StartFrame(), dialog.StopFrame());
  }
}

//-----------------------------------------------------------------------------
bool lqSaveLidarFrameReaction::GetFolderAndBaseNameFromUser()
{
  pqFileDialog dialog(nullptr, pqCoreUtilities::mainWidget(), tr("Save File"));
  dialog.selectFile(this->FolderPath + "/" + this->BaseName);
  dialog.setFileMode(pqFileDialog::AnyFile);

  QString saveFileName;
  if (dialog.exec())
  {
    saveFileName = dialog.getSelectedFiles(0).first();
  }

  if (saveFileName.isEmpty())
  {
    return false;
  }

  QFileInfo fileInfo(saveFileName);
  this->FolderPath = fileInfo.path();
  this->BaseName = fileInfo.baseName();
  return true;
}

//-----------------------------------------------------------------------------
bool lqSaveLidarFrameReaction::saveFrame(vtkSMProxy* lidar, int start, int stop)
{
  if (!GetFolderAndBaseNameFromUser())
  {
    return false;
  }

  auto* lidarProxy = vtkSMSourceProxy::SafeDownCast(lidar);

  // Create writer asked by the action
  vtkSmartPointer<vtkSMSourceProxy> writer = this->CreateWriter(lidarProxy);
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

  // Set precision (especially for csv writer) to write timestamp
  // this value is ignored if the writer does not have the precision property.
  vtkSMPropertyHelper(writer, "Precision", true).Set(13);
  writer->UpdateVTKObjects();

  // 3 iterate over the frame
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene = mgr->getActiveScene();
  double timestep_bakup = scene->getAnimationTime();

  if (start == -1 && stop == -1)
  {
    QString filename =
      this->FolderPath + "/" + this->GenerateFileName(this->BaseName, timestep_bakup);
    vtkSMPropertyHelper(writer, "FileName").Set(filename.toStdString().c_str());
    writer->UpdateVTKObjects();
    writer->UpdatePipeline(timestep_bakup);
  }
  else
  {
    auto* tsv = vtkSMDoubleVectorProperty::SafeDownCast(lidarProxy->GetProperty("TimestepValues"));
    QProgressDialog progress("Saving files...", "Abort", 0, stop - start);
    progress.setWindowModality(Qt::ApplicationModal);
    for (int i = start; i <= stop; ++i)
    {
      double timestep = tsv->GetElement(i);
      scene->setAnimationTime(timestep);
      QString filename = this->FolderPath + "/" + this->GenerateFileName(this->BaseName, timestep);

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
  // format filename in a way that is readable again as a time sequence when re-opening the data
  QString timeInfo = "_" + QString::number(GetFrameIndexOfTimestamp(timestep));

  QString sFilenameMainString = baseName + timeInfo;

  QString sFilenameWithoutExt = sFilenameMainString;
  // Add suffix to the new file in case they already exist with the base name we would like
  // to give it
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
vtkSmartPointer<vtkSMSourceProxy> lqSaveLidarFrameReaction::CreateWriter(
  vtkSMSourceProxy* lidarProxy)
{
  vtkSMWriterFactory* writerFactory = vtkSMProxyManager::GetProxyManager()->GetWriterFactory();
  auto* tmp = vtkSMSourceProxy::SafeDownCast(
    writerFactory->CreateWriter(this->WriterName.toStdString().c_str(), lidarProxy, 0, true));
  return vtkSmartPointer<vtkSMSourceProxy>(tmp);
}
