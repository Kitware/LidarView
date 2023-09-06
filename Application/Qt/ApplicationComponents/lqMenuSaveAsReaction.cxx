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
}

//-----------------------------------------------------------------------------
void lqMenuSaveAsReaction::onUpdateUI(pqPipelineSource*)
{
  bool hasLidarReader = HasProxy<vtkLidarReader>();
  this->parentMenu()->setEnabled(hasLidarReader);
}
