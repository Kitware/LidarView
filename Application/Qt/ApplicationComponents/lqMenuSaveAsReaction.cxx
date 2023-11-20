/*=========================================================================

  Program: LidarView
  Module:  lqMenuSaveAsReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqMenuSaveAsReaction.h"

#include <pqPVApplicationCore.h>

#include "lqHelper.h"
#include "lqSaveLidarFrameReaction.h"

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
lqMenuSaveAsReaction::lqMenuSaveAsReaction(QMenu* parentObject)
  : Superclass(parentObject)
{
  auto* core = pqApplicationCore::instance();

  pqServerManagerModel* smmodel = core->getServerManagerModel();
  this->connect(
    smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onUpdateUI(pqPipelineSource*)));
  this->connect(
    smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(onUpdateUI(pqPipelineSource*)));

  this->onUpdateUI(nullptr);
  this->populateMenu();
}

//-----------------------------------------------------------------------------
void lqMenuSaveAsReaction::onUpdateUI(pqPipelineSource*)
{
  bool hasLidarReader = HasProxy<vtkLidarReader>();
  this->parentMenu()->setEnabled(hasLidarReader);
}

//-----------------------------------------------------------------------------
void lqMenuSaveAsReaction::populateMenu()
{
  std::vector<std::string> supportedExtensions = { "pcd", "csv", "ply", "las" };
  std::vector<std::string> writers = {
    "PCDWriter", "DataSetCSVWriter", "PPLYWriter", "PLASWriter"
  };

  for (unsigned int i = 0; i < supportedExtensions.size(); i++)
  {
    const char* writer = writers[i].c_str();
    std::string label = "Save as " + supportedExtensions[i] + "...";
    std::string actionName = "actionSave" + supportedExtensions[i];
    QAction* actn = new QAction(label.c_str());
    actn->setObjectName(actionName.c_str());
    new lqSaveLidarFrameReaction(actn, writer, supportedExtensions[i].c_str());
    this->parentMenu()->addAction(actn);
  }
}