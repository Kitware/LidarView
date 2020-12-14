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
