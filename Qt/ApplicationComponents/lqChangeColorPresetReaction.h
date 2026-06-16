/*=========================================================================

  Program: LidarView
  Module:  lqChangeColorPresetReaction.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqChangeColorPresetReaction_h
#define lqChangeColorPresetReaction_h

#include "lqApplicationComponentsModule.h"

#include <QAction>
#include <QObject>
#include <QPointer>

#include <pqReaction.h>

class QMenu;
class pqDataRepresentation;
class pqTimer;

/**
 * @ingroup Reactions
 * Reaction to create a menu with scalar color preset to do a quick change.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqChangeColorPresetReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  lqChangeColorPresetReaction(QAction* parent);
  ~lqChangeColorPresetReaction() override;

public Q_SLOTS:
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

  /**
   * Set the data representation explicitly when track_active_objects is false.
   */
  void setRepresentation(pqDataRepresentation* repr);

private Q_SLOTS:
  /**
   * Forces a re-population of the menu.
   */
  void populateMenu();

  /**
   * Callback when a menu action is triggered.
   */
  void actionTriggered(QAction* actn);

private:
  Q_DISABLE_COPY(lqChangeColorPresetReaction)
  QPointer<QMenu> Menu;
  QPointer<pqDataRepresentation> Representation;
  pqTimer* Timer;
};

#endif
