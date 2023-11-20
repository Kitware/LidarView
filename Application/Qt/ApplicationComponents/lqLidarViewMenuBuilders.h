/*=========================================================================

  Program: LidarView
  Module:  lqLidarViewMenuBuilders.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqLidarViewMenuBuilders_h
#define lqLidarViewMenuBuilders_h

class pqPropertiesPanel;
class QMenu;
class QMainWindow;
class QString;

#include "lvApplicationComponentsModule.h"

/**
 * lqLidarViewMenuBuilders provides helper methods to build menus that are
 * exactly as used by LidarView client. Simply call the appropriate method with
 * the menu as an argument, and it will be populated with actions and reactions
 * for standard LidarView behavior.
 */
class LVAPPLICATIONCOMPONENTS_EXPORT lqLidarViewMenuBuilders
{
public:
  /**
   * Builds standard File menu.
   */
  static void buildFileMenu(QMenu& menu);

  /**
   * Builds standard Edit menu.
   */
  static void buildEditMenu(QMenu& menu, pqPropertiesPanel* propertiesPanel);

  /**
   * Builds standard Help menu.
   */
  static void buildHelpMenu(QMenu& menu);

  /**
   * Builds and adds all standard LidarView toolbars.
   */
  static void buildToolbars(QMainWindow& mainWindow);
};

#endif
