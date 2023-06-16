/*=========================================================================

  Program:   LidarView
  Module:    lqAboutDialogReaction.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqAboutDialogReaction.h"

#include "lqAboutDialog.h"
#include "pqCoreUtilities.h"

//-----------------------------------------------------------------------------
lqAboutDialogReaction::lqAboutDialogReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void lqAboutDialogReaction::showAboutDialog()
{
  lqAboutDialog about_dialog(pqCoreUtilities::mainWidget());
  about_dialog.exec();
}
