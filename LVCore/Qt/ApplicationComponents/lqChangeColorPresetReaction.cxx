/*=========================================================================

  Program: LidarView
  Module:  lqChangeColorPresetReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqChangeColorPresetReaction.h"

#include <vtkSMColorMapEditorHelper.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMTransferFunctionPresets.h>
#include <vtkSMTransferFunctionProxy.h>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqPresetGroupsManager.h>
#include <pqTimer.h>
#include <pqUndoStack.h>

#include <QMenu>

//-----------------------------------------------------------------------------
lqChangeColorPresetReaction::lqChangeColorPresetReaction(QAction* parent)
  : Superclass(parent)
  , Timer(new pqTimer(this))
{

  QMenu* menu = new QMenu();
  this->Menu = menu;
  menu->setObjectName("LoadColorPresetMenu");
  parent->setMenu(menu);
  this->connect(menu, SIGNAL(aboutToShow()), SLOT(populateMenu()));
  this->connect(menu, SIGNAL(triggered(QAction*)), SLOT(actionTriggered(QAction*)));

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)),
    this,
    SLOT(setRepresentation(pqDataRepresentation*)));
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());

  this->Timer->setSingleShot(true);
  this->Timer->setInterval(0);
  this->connect(this->Timer, SIGNAL(timeout()), SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
lqChangeColorPresetReaction::~lqChangeColorPresetReaction()
{
  if (QAction* pa = this->parentAction())
  {
    pa->setMenu(nullptr);
  }
  delete this->Menu;
}

//-----------------------------------------------------------------------------
void lqChangeColorPresetReaction::setRepresentation(pqDataRepresentation* repr)
{
  if (this->Representation)
  {
    this->Representation->disconnect(this->Timer);
    this->Timer->disconnect(this->Representation);
  }
  this->Representation = repr;
  if (repr)
  {
    this->Timer->connect(repr, SIGNAL(colorTransferFunctionModified()), SLOT(start()));
    this->Timer->connect(repr, SIGNAL(colorArrayNameModified()), SLOT(start()));
  }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void lqChangeColorPresetReaction::updateEnableState()
{
  vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : nullptr;
  bool enabled = reprProxy && vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy);
  this->parentAction()->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void lqChangeColorPresetReaction::populateMenu()
{
  QMenu* menu = qobject_cast<QMenu*>(this->sender());
  menu->clear();

  auto transferFunctionPresets = vtkSMTransferFunctionPresets::GetInstance();
  auto groupManager = qobject_cast<pqPresetGroupsManager*>(
    pqApplicationCore::instance()->manager("PRESET_GROUP_MANAGER"));

  for (unsigned int index = 0; index < transferFunctionPresets->GetNumberOfPresets(); ++index)
  {
    auto presetName = QString::fromStdString(transferFunctionPresets->GetPresetName(index));
    if (groupManager->presetRankInGroup(presetName, "Default") != -1)
    {
      menu->addAction(presetName);
    }
  }
}

//-----------------------------------------------------------------------------
void lqChangeColorPresetReaction::actionTriggered(QAction* actn)
{
  vtkSMProxy* proxy = this->Representation->getProxy();
  assert(proxy);

  BEGIN_UNDO_SET(tr("Apply color preset"));
  if (vtkSMProxy* lutProxy = vtkSMPropertyHelper(proxy, "LookupTable").GetAsProxy())
  {
    auto presetName = actn->text().toStdString();
    vtkSMTransferFunctionProxy::ApplyPreset(lutProxy, presetName.c_str());
  }
  END_UNDO_SET();
}
