#include "lqOpenPcapReaction.h"

#include <vtkNew.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>

#include <pqActiveObjects.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqObjectBuilder.h>
#include <pqPVApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqSettings.h>
#include <pqView.h>

#include "lqHelper.h"
#include "lqLidarViewManager.h"
#include "lqSensorListWidget.h"
#include "lqUpdateCalibrationReaction.h"
#include "lqCalibrationDialog.h"

#include <QApplication>
#include <QProgressDialog>
#include <QString>

#include <string>

#include <vtkCommand.h>

#include <vtkPVProgressHandler.h>
#include <vtkPVSession.h>
#include <vtkProcessModule.h>

//----------------------------------------------------------------------------
class lqOpenPcapReaction::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
  {
    vtkObserver* obs = new vtkObserver();
    return obs;
  }

  void Execute(vtkObject*, unsigned long eventId, void*) override
  {

    if (eventId == vtkCommand::ProgressEvent)
    {
      QApplication::instance()->processEvents();
    }
  }
};

//-----------------------------------------------------------------------------
lqOpenPcapReaction::lqOpenPcapReaction(QAction* action)
  : Superclass(action)
{
}

//-----------------------------------------------------------------------------
void lqOpenPcapReaction::onTriggered()
{
  // Get the pcap filename
  pqSettings* settings = pqApplicationCore::instance()->settings();
  const char* defaultDir_path = "LidarPlugin/OpenData/DefaultDir";
  QString defaultDir = settings->value(defaultDir_path, QDir::homePath()).toString();

  pqFileDialog dial(pqActiveObjects::instance().activeServer(),
    pqCoreUtilities::mainWidget(),
    tr("Open LiDAR File"),
    defaultDir,
    tr("Wireshark Capture (*.pcap)"));
  dial.setObjectName("LidarFileOpenDialog");
  dial.setFileMode(pqFileDialog::ExistingFile);
  if (dial.exec() == QDialog::Rejected)
  {
    return;
  }
  QString fileName = dial.getSelectedFiles().at(0);
  QFileInfo fileInfo(fileName);
  settings->setValue(defaultDir_path, fileInfo.absolutePath());

  lqOpenPcapReaction::createSourceFromFile(fileName);
}

//-----------------------------------------------------------------------------
void lqOpenPcapReaction::createSourceFromFile(QString fileName)
{
  // Launch the calibration Dialog before creating the Source to allow to cancel the action
  // (with the "cancel" button in the dialog)
  lqCalibrationDialog dialog(lqLidarViewManager::instance()->getMainWindow(), false);
  // DisplayDialogOnActiveWindow(dialog);
  if (dialog.exec())
  {
    lqOpenPcapReaction::createSourceFromFile(fileName, dialog);
  }
}

//-----------------------------------------------------------------------------
void lqOpenPcapReaction::createSourceFromFile(QString fileName, const lqCalibrationDialog& dialog)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  pqView* view = pqActiveObjects::instance().activeView();

  // Create a progress bar so the user see that LidarView is running
  QProgressDialog progress("Reading pcap", "", 0, 0, lqLidarViewManager::getMainWindow());
  progress.setCancelButton(nullptr);
  progress.setModal(true);
  progress.show();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVSession* session = vtkPVSession::SafeDownCast(pm->GetSession());
  if (!session)
  {
    return;
  }
  vtkSmartPointer<vtkPVProgressHandler> handler = session->GetProgressHandler();
  handler->PrepareProgress();
  double interval = handler->GetProgressInterval();
  handler->SetProgressInterval(0.05);
  vtkNew<vtkObserver> obs;
  unsigned long tag = handler->AddObserver(vtkCommand::ProgressEvent, obs);

  // Remove all Streams (and every filter depending on them) from pipeline Browser
  // Thanks to the lqSensorListWidget,
  // if a LidarStream is delete, it will automatically delete its PositionOrientationStream.
  // So we just have to delete all lidarStream.
  RemoveAllProxyTypeFromPipelineBrowser<vtkLidarStream>();

  if (!dialog.isEnableMultiSensors())
  {
    // We remove all lidarReader (and every filter depending on them) in the pipeline
    // Thanks to the lqSensorListWidget,
    // if a LidarReader is delete, it will automatically delete its PositionOrientationReader.
    // So we just have to delete all lidarReader.
    RemoveAllProxyTypeFromPipelineBrowser<vtkLidarReader>();
  }

  // Create the lidar Reader
  // We have to use pqObjectBuilder::createSource to add the created source to the pipeline
  // The source will be created immediately so the signal "sourceAdded" of the pqServerManagerModel
  // is send during "create source".
  // To get the pqPipelineSource modified with the new property, you have to connect to the signal
  // "dataUpdated" of the pqServerManagerModel
  pqPipelineSource* lidarSource = builder->createSource("sources", "LidarReader", server);
  lidarSource->setModifiedState(pqProxy::UNMODIFIED);
  vtkSMPropertyHelper(lidarSource->getProxy(), "FileName").Set(fileName.toStdString().c_str());
  lidarSource->getProxy()->UpdateProperty("FileName");

  // For Hesai, enable advanced arrays by default
  vtkSMProxy* interpProxy =
    vtkSMPropertyHelper(lidarSource->getProxy(), "PacketInterpreter").GetAsProxy();
  vtkSmartPointer<vtkLidarPacketInterpreter> interp =
    vtkLidarPacketInterpreter::SafeDownCast(interpProxy->GetClientSideObject());
  unsigned int found = interp->GetSensorInformation().find("Hesai");
  if (found != std::string::npos)
  {
    vtkSMPropertyHelper(interpProxy, "EnableAdvancedArrays").Set(1);
    interpProxy->UpdateProperty("PacketInterpreter");
    lidarSource->getProxy()->UpdateProperty("PacketInterpreter");
  }
  QString lidarName = lidarSource->getSMName();

  pqPipelineSource* posOrSource = nullptr;
  QString posOrName = "";

  // Update lidarSource and posOrSource
  // If the GPs interpretation is asked, the posOrsource will be created in the
  // lqUpdateCalibrationReaction because it has to manage it if the user enable interpreting GPS
  // packet after the first instantiation
  lqUpdateCalibrationReaction::UpdateCalibration(lidarSource, posOrSource, dialog);

  if (posOrSource)
  {
    posOrSource->updatePipeline();
    posOrName = posOrSource->getSMName();
    controller->Show(posOrSource->getSourceProxy(), 0, view->getViewProxy());
  }

  // Update applogic to be able to use function only define in applogic.
  lqLidarViewManager::instance()->runPython(
    QString("lv.UpdateApplogicReader('%1', '%2')\n").arg(lidarName, posOrName));

  // Show the Lidar source
  controller->Show(lidarSource->getSourceProxy(), 0, view->getViewProxy());
  pqActiveObjects::instance().setActiveSource(lidarSource);
  pqApplicationCore::instance()->render();

  // WIP Workaround Seek to first Frame To prevent displaying half frame from begining
  pqAnimationScene* animScene =
    pqPVApplicationCore::instance()->animationManager()->getActiveScene();
  if (animScene)
  {
    QList<double> timesteps = animScene->getTimeSteps();
    if (!timesteps.empty())
    {
      animScene->setAnimationTime(timesteps.first());
    }
  }

  // Remove the handler so the user can interact with LidarView again (pushing any button)
  handler->RemoveObserver(tag);
  handler->LocalCleanupPendingProgress();
  handler->SetProgressInterval(interval);
  progress.close();
}
