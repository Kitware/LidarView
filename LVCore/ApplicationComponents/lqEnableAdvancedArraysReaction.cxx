#include "lqEnableAdvancedArraysReaction.h"

#include "lqHelper.h"

#include <QFileDialog>
#include <QMessageBox>

#include <pqApplicationCore.h>
#include <pqFileDialog.h>
#include <pqServerManagerModel.h>
#include <pqPipelineSource.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMPropertyHelper.h>

//-----------------------------------------------------------------------------
lqEnableAdvancedArraysReaction::lqEnableAdvancedArraysReaction(QAction *action)
  : Superclass(action)
{
  this->parentAction()->setEnabled(false);
  auto* core = pqApplicationCore::instance();

  pqServerManagerModel* smmodel = core->getServerManagerModel();
  this->connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onSourceAdded(pqPipelineSource*)));
  this->connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(onSourceRemoved(pqPipelineSource*)));

  foreach (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
    this->onSourceAdded(src);

  this->parentAction()->setCheckable(true);
}

//-----------------------------------------------------------------------------
void lqEnableAdvancedArraysReaction::updateIcon(bool setEnable)
{
  if(setEnable)
  {
    // Set the icon on "Enable advanced Arrays"
    this->parentAction()->setIcon(QIcon(":/lqResources/Icons/EnableAdvancedArrays.png"));
    this->parentAction()->setToolTip(QString("Enable the interpreter's advanced arrays."));
  }
  else
  {
    // Set the icon on "Disable advanced Arrays"
    this->parentAction()->setIcon(QIcon(":/lqResources/Icons/DisableAdvancedArrays.png"));
    this->parentAction()->setToolTip(QString("Disable the interpreter's advanced arrays."));
  }
}

//-----------------------------------------------------------------------------
void lqEnableAdvancedArraysReaction::onTriggered()
{
  // The property should be set to true if the button is "checked
  int booleanToSet = this->parentAction()->isChecked();

  // The Ui should be update to the next available action
  this->updateIcon(!this->parentAction()->isChecked());

  // Update all lidar proxy with the new property value
  std::vector<vtkSMProxy*> lidarProxys = GetLidarsProxy();
  for(vtkSMProxy* proxy : lidarProxys)
  {
    vtkSMProxy* interp = SearchProxyByGroupName(proxy, "LidarPacketInterpreter");
    if(interp != nullptr)
    {
      vtkSMProperty* property = GetPropertyFromProxy(interp, "EnableAdvancedArrays");
      vtkSMPropertyHelper(property).Set(booleanToSet);

      //Update the proxy
      proxy->UpdateSelfAndAllInputs();
      vtkSMSourceProxy * sourcelidarProxy = vtkSMSourceProxy::SafeDownCast(proxy);
      if(sourcelidarProxy)
      {
        sourcelidarProxy->UpdatePipelineInformation();
      }
      pqApplicationCore::instance()->render();
    }
  }
}

//-----------------------------------------------------------------------------
void lqEnableAdvancedArraysReaction::onSourceAdded(pqPipelineSource *src)
{
  if (!this->parentAction()->isEnabled() && IsLidarProxy(src->getProxy()))
  {
    this->parentAction()->setEnabled(true);

    // If the "Enable advanced Array" property is ON by default on a specific interpreter
    // The first action of the button will be "Enable Advanced Arrays" and it will do nothing
    // So the "Enable/Disable" button should be initialize according to the default value
    // of the property "Enable Advanced Array" of the interpreter
    std::vector<vtkSMProxy*> lidarProxys = GetLidarsProxy();
    if(lidarProxys.empty())
    {
      return;
    }
    vtkSMProxy* proxy = lidarProxys[0];

    // The Ui is updated, each time the Interpreter of the first Lidar Proxy is Modified
    this->Connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->Connection->Connect(
      proxy->GetProperty("PacketInterpreter"), vtkCommand::ModifiedEvent, this, SLOT(updateUI()));

    this->updateUI();
  }
}

//-----------------------------------------------------------------------------
void lqEnableAdvancedArraysReaction::updateUI()
{
  // The choice of the button "Enable" or "Disable" is based on the first lidar of the pipeline
  std::vector<vtkSMProxy*> lidarProxys = GetLidarsProxy();
  if(lidarProxys.empty())
  {
    return;
  }
  vtkSMProxy* proxy = lidarProxys[0];

  vtkSMProxy* interp = SearchProxyByGroupName(proxy, "LidarPacketInterpreter");
  if(interp != nullptr)
  {
    vtkSMProperty* property = GetPropertyFromProxy(interp, "EnableAdvancedArrays");
    bool enableAdvancedArrays = vtkSMPropertyHelper(property).GetAsInt();

    // The Ui should be updated to the next available action : "disable/enable..."
    // Check/Uncheck the "Enable Advanced Array" button
    this->parentAction()->setChecked(enableAdvancedArrays);
    this->updateIcon(!enableAdvancedArrays);
  }
}

//-----------------------------------------------------------------------------
void lqEnableAdvancedArraysReaction::onSourceRemoved(pqPipelineSource * vtkNotUsed(src)){
  if (this->parentAction()->isEnabled() && !HasLidarProxy())
  {
    this->parentAction()->setEnabled(false);
    this->parentAction()->setChecked(false);
    this->parentAction()->setIcon(QIcon(":/lqResources/Icons/EnableAdvancedArrays.png"));
  }
}
