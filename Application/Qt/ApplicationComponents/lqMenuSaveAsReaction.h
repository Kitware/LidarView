/*=========================================================================

  Program: LidarView
  Module:  lqMenuSaveAsReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqMenuSaveAsReaction_h
#define lqMenuSaveAsReaction_h

#include "lvApplicationComponentsModule.h"

#include <QMenu>
#include <QObject>

class pqPipelineSource;

/**
 * @ingroup Reactions
 * Reaction to save data files.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqMenuSaveAsReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  lqMenuSaveAsReaction(QMenu* parent);

  /**
   * Provides access to the parent menu.
   */
  QMenu* parentMenu() const { return qobject_cast<QMenu*>(this->parent()); }

public Q_SLOTS:
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void onUpdateUI(pqPipelineSource*);

private:
  Q_DISABLE_COPY(lqMenuSaveAsReaction)
};

#endif
