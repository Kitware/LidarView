/*=========================================================================

  Program: LidarView
  Module:  lqPythonShellReaction.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqPythonShellReaction.h"

#include <QDockWidget>

#include <pqApplicationCore.h>

//-----------------------------------------------------------------------------
lqPythonShellReaction::lqPythonShellReaction(QAction* parent)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
lqPythonShellReaction::~lqPythonShellReaction() = default;

//-----------------------------------------------------------------------------
void lqPythonShellReaction::togglePythonShellPanel()
{
  // Raise the python shell if present in the application.
  QDockWidget* widget =
    qobject_cast<QDockWidget*>(pqApplicationCore::instance()->manager("PYTHON_SHELL_PANEL"));
  if (widget)
  {
    bool show = !widget->isVisible();
    widget->setVisible(show);
    if (show)
    {
      widget->raise();
    }
  }
  else
  {
    qDebug("Failed to find 'PYTHON_SHELL_PANEL'.");
  }
}
