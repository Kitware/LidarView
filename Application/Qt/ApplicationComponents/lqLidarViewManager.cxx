/*=========================================================================

  Program:   LidarView
  Module:    lqLidarViewManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLidarViewManager.h"

#include "lqLivePlayerWidget.h"

#include <vtkNew.h>
#include <vtkPVGeneralSettings.h>
#include <vtkRemotingCoreConfiguration.h>
#include <vtkResourceFileLocator.h>
#include <vtkSMSettings.h>
#include <vtksys/FStream.hxx>

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <QDebug>
#include <QDockWidget>
#include <QFileInfo>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QToolBar>
#include <QVariant>

#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>
#include <pqSettings.h>
#include <pqTabbedMultiViewWidget.h>

#include <array>
#include <map>
#include <string>

namespace
{
static const std::string INTERFACE_CONFIGURATION_FILENAME = "interface_modes_config.json";

//-----------------------------------------------------------------------------
void setSetting(vtkSMSettings* settings, std::string name, double value)
{
  settings->SetSetting(name.c_str(), value);
}

//-----------------------------------------------------------------------------
std::string getConfigName(lqLidarViewManager::InterfaceModes mode)
{
  switch (mode)
  {
    case lqLidarViewManager::InterfaceModes::LIDAR_VIEWER:
      return "lidarViewer";
    case lqLidarViewManager::InterfaceModes::POINT_CLOUD_TOOL:
      return "pointCloudTool";
    case lqLidarViewManager::InterfaceModes::ADVANCED_MODE:
      return "advancedMode";
  }
  return "";
}
}

//------------------------------------------------------------------------------
struct lqLidarViewManager::lqInternals
{
private:
  //-----------------------------------------------------------------------------
  /**
   * Return the path to the interface configuration json file, located in the share/ folder,
   * found relatively to the path of the application executable being run.
   */
  std::string getInterfaceConfigFileName()
  {
    std::string interfaceConfigFileName;
    std::string appLocation = pqCoreUtilities::getParaViewApplicationDirectory().toStdString();
    std::vector<std::string> prefixes{ "share" };
    vtkNew<vtkResourceFileLocator> locator;
    std::string basePath = locator->Locate(appLocation, prefixes, INTERFACE_CONFIGURATION_FILENAME);
    return vtksys::SystemTools::CollapseFullPath(INTERFACE_CONFIGURATION_FILENAME, basePath);
  }

  //-----------------------------------------------------------------------------
  template <typename T>
  T* findObjectByObjectName(QList<T*> objectList, std::string name)
  {
    for (auto object : objectList)
    {
      if (object->objectName() == name.c_str())
      {
        return object;
      }
    }
    return nullptr;
  }

  //-----------------------------------------------------------------------------
  bool isObjectInList(QObject* object, std::vector<std::string> list)
  {
    return std::find(list.begin(), list.end(), object->objectName().toStdString()) != list.end();
  }

  //-----------------------------------------------------------------------------
  /**
   * Populate a QMenu with actions from the menuBase using the specified list of action names.
   */
  void fillMenu(QMenu* menuBase, QMenu* menu, const std::vector<std::string> actions)
  {
    for (const std::string& actionName : actions)
    {
      if (actionName == "separator")
      {
        menu->addSeparator();
      }
      else if (auto action = menuBase->findChild<QAction*>(actionName.c_str()))
      {
        menu->addAction(action);
      }
      else if (QMenu* submenu = menuBase->findChild<QMenu*>(actionName.c_str()))
      {
        menu->addMenu(submenu);
      }
      else
      {
        qWarning() << "Menu entry <" << actionName.c_str() << "> not found.";
      }
    }
  }

  //-----------------------------------------------------------------------------
  /**
   * Hide all toolbar actions not specified in action name list.
   */
  void filterToolbar(QToolBar* toolbar, const std::vector<std::string> actionList)
  {
    bool wasSeparator = true;
    for (auto action : toolbar->actions())
    {
      std::string actionName = action->objectName().toStdString();
      if (actionName.empty() && !wasSeparator)
      {
        action->setVisible(!wasSeparator);
        wasSeparator = true;
      }
      else
      {
        bool visible = this->isObjectInList(action, actionList);
        action->setVisible(visible);
        wasSeparator = wasSeparator && !visible;
      }
    }
  }

  typedef QMap<QString, QVariant> ModeSettingsType;

  std::map<lqLidarViewManager::InterfaceModes, ModeSettingsType> modesSettings;
  nlohmann::json baseJsonConfig;
  nlohmann::json currentJsonMode;
  lqLidarViewManager::InterfaceModes currentMode;

public:
  QMainWindow* mainWindow;

  //-----------------------------------------------------------------------------
  lqInternals()
  {
    vtksys::ifstream interfaceConfigFile(this->getInterfaceConfigFileName().c_str());
    if (!interfaceConfigFile.is_open())
    {
      QMessageBox messageBox;
      messageBox.critical(
        nullptr, "Error", "Could not open interface_modes_config.json.\nLidarView cannot start.");
      assert(interfaceConfigFile.is_open());
    }

    this->baseJsonConfig = nlohmann::json::parse(interfaceConfigFile);
    this->setInterfaceMode(lqLidarViewManager::LIDAR_VIEWER);
  }

  //-----------------------------------------------------------------------------
  void setInterfaceMode(lqLidarViewManager::InterfaceModes mode)
  {
    this->currentMode = mode;
    this->currentJsonMode = this->baseJsonConfig[::getConfigName(mode)];
  }

  //-----------------------------------------------------------------------------
  void updateMenusLayout()
  {
    // Change display for menus
    QList<QMenu*> menus = this->mainWindow->findChildren<QMenu*>();
    nlohmann::json menusConfig = this->currentJsonMode["menus"];
    for (nlohmann::json::iterator it = menusConfig.begin(); it != menusConfig.end(); ++it)
    {
      std::string menuName = it.key();
      std::vector<std::string> showMenus(it.value().begin(), it.value().end());

      if (menuName == "hide")
      {
        for (auto menu : menus)
        {
          bool visible = !this->isObjectInList(menu, showMenus);
          menu->menuAction()->setVisible(visible);
        }
        continue;
      }

      QMenu* menu = this->findObjectByObjectName(menus, menuName);
      if (!menu)
      {
        qWarning() << "lqLidarViewManager::updateInterfaceLayout: Could not find \""
                   << menuName.c_str() << "\" menu.";
        continue;
      }

      QMenu* menuBase = this->findObjectByObjectName(menus, menuName + "Base");
      if (menu && menuBase)
      {
        menu->clear();
        this->fillMenu(menuBase, menu, showMenus);
      }
    }
  }

  //-----------------------------------------------------------------------------
  void updateToolBarsLayout()
  {
    QList<QToolBar*> toolbars = this->mainWindow->findChildren<QToolBar*>();
    nlohmann::json toolBarsConfig = this->currentJsonMode["toolbars"];

    if (toolBarsConfig["show"].is_array())
    {
      std::vector<std::string> toolbarsToShow(
        toolBarsConfig["show"].begin(), toolBarsConfig["show"].end());
      for (auto toolbar : toolbars)
      {
        bool visible = this->isObjectInList(toolbar, toolbarsToShow);
        toolbar->setVisible(visible);
        // Set visible all actions in toolbars
        for (auto action : toolbar->actions())
        {
          action->setVisible(true);
        }
      }
    }

    for (nlohmann::json::iterator it = toolBarsConfig.begin(); it != toolBarsConfig.end(); ++it)
    {
      std::string toolbarName = it.key();
      if (toolbarName == "show" || toolbarName == "toolbarBreaks")
      {
        continue;
      }

      QToolBar* toolbar = this->findObjectByObjectName(toolbars, toolbarName);
      if (!toolbar)
      {
        qWarning() << "lqLidarViewManager::updateInterfaceLayout: Could not find \""
                   << toolbarName.c_str() << "\" toolbar.";
        continue;
      }
      this->filterToolbar(toolbar, std::vector<std::string>(it.value().begin(), it.value().end()));
    }

    if (toolBarsConfig["toolbarBreaks"].is_array())
    {
      for (auto toolbar : toolbars)
      {
        this->mainWindow->removeToolBarBreak(toolbar);
      }
      for (std::string name : toolBarsConfig["toolbarBreaks"])
      {
        QToolBar* toolbar = this->findObjectByObjectName(toolbars, name);
        this->mainWindow->insertToolBarBreak(toolbar);
      }
    }
  }

  //-----------------------------------------------------------------------------
  void updateDockWidgetsLayout()
  {
    QList<QDockWidget*> dockWidgets = this->mainWindow->findChildren<QDockWidget*>();
    nlohmann::json dockWidgetsConfig = this->currentJsonMode["dockWidgets"];
    std::vector<std::string> defaultOpenDocks(dockWidgetsConfig.begin(), dockWidgetsConfig.end());
    for (auto widget : dockWidgets)
    {
      bool visible = this->isObjectInList(widget, defaultOpenDocks);
      widget->setVisible(visible);
    }
  }

  //-----------------------------------------------------------------------------
  void updateMultiViewWidgetVisibility()
  {
    auto tmvWidget = qobject_cast<pqTabbedMultiViewWidget*>(
      pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
    tmvWidget->setDecorationsVisibility(this->currentJsonMode["showFrameDecoration"]);
  }

  //-----------------------------------------------------------------------------
  void updateShowScalarsBarsSettings()
  {
    vtkPVGeneralSettings* gsSettings = vtkPVGeneralSettings::GetInstance();
    if (this->currentJsonMode["automaticallyShowScalarBars"])
    {
      gsSettings->SetScalarBarMode(vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS);
    }
    else
    {
      gsSettings->SetScalarBarMode(vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS);
    }
  }

  //-----------------------------------------------------------------------------
  void updateLidarPlayerSpeedMode(InterfaceModes interfaceMode)
  {
    QDockWidget* lidarPlayer =
      qobject_cast<QDockWidget*>(pqApplicationCore::instance()->manager("LIDAR_PLAYER_PANEL"));
    if (lidarPlayer)
    {
      lqLivePlayerWidget* widget = qobject_cast<lqLivePlayerWidget*>(lidarPlayer->widget());
      if (widget)
      {
        auto playMode = interfaceMode == lqLidarViewManager::LIDAR_VIEWER
          ? lqLiveVCRController::EMULATED_TIME
          : lqLiveVCRController::ALL_FRAMES;
        widget->changeReaderMode(playMode);
      }
    }
  }

  //-----------------------------------------------------------------------------
  void saveModeState()
  {
    ModeSettingsType& settings = this->modesSettings[this->currentMode];
    settings.insert("Size", this->mainWindow->size());
    settings.insert("Layout", this->mainWindow->saveState());
  }

  //-----------------------------------------------------------------------------
  void restoreModeState()
  {
    ModeSettingsType& settings = this->modesSettings[this->currentMode];

    if (settings.contains("Size"))
    {
      this->mainWindow->resize(settings.take("Size").toSize());
    }

    if (settings.contains("Layout"))
    {
      this->mainWindow->restoreState(settings.take("Layout").toByteArray());
    }
  }
};

//-----------------------------------------------------------------------------
lqLidarViewManager::lqLidarViewManager(QObject* parent /*=nullptr*/)
  : Superclass(parent)
  , Internals(new lqLidarViewManager::lqInternals())
{
  auto& intern = *this->Internals;
  intern.mainWindow = qobject_cast<QMainWindow*>(this->parent());
  if (!intern.mainWindow)
  {
    qWarning("lqLidarViewManager::updateInterfaceLayout: Could not cast main window.");
    return;
  }
}

//-----------------------------------------------------------------------------
lqLidarViewManager::~lqLidarViewManager() = default;

//-----------------------------------------------------------------------------
void lqLidarViewManager::setLidarViewDefaultSettings()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  // By default add an empty RenderView which will be remplace by a LidarGridView
  // at LidarCorePlugin loading
  settings->SetSetting("settings.GeneralSettings.DefaultViewType", "Empty");

  // Set default LUT for lidar intensity/reflectivity
  const std::array<std::string, 2> LUTName = { "intensity", "reflectivity" };
  const double LUTValue[12] = { 0.0, 0.0, 0.0, 1.0, 100.0, 1.0, 1.0, 0.0, 256.0, 1.0, 0.0, 0.0 };

  for (const auto& name : LUTName)
  {
    const std::string settingName = "array_lookup_tables." + name + ".PVLookupTable.RGBPoints";
    for (unsigned short i_val = 0; i_val < 12; i_val++)
    {
      settings->SetSetting(settingName.c_str(), i_val, LUTValue[i_val]);
    }
    ::setSetting(settings, "array_lookup_tables." + name + ".PVLookupTable.ColorSpace", 1);
  }

  // Set default LUT for dual return lidar scalars
  const std::string drLUTName[2] = { "dual_intensity", "dual_distance" };
  const double drLUTValues[2][12] = {
    { -1.0, 0.5, 0.2, 0.8, 0.0, 0.6, 0.6, 0.6, 1.0, 1.0, 0.9, 0.4 },
    { -1.0, 0.1, 0.5, 0.7, 0.0, 0.9, 0.9, 0.9, 1.0, 0.8, 0.2, 0.3 }
  };
  const std::string drLUTAnnotations[2][6] = { { "-1", "low", "0", "dual", "1", "high" },
    { "-1", "near", "0", "dual", "1", "far" } };

  for (unsigned short i_name = 0; i_name < 2; i_name++)
  {
    const std::string name = drLUTName[i_name];
    const std::string settingRGB = "array_lookup_tables." + name + ".PVLookupTable.RGBPoints";
    const std::string settingAnnot = "array_lookup_tables." + name + ".PVLookupTable.Annotations";
    for (unsigned short i_val = 0; i_val < 12; i_val++)
    {
      settings->SetSetting(settingRGB.c_str(), i_val, drLUTValues[i_name][i_val]);
    }
    for (unsigned short i_annot = 0; i_annot < 6; i_annot++)
    {
      settings->SetSetting(settingAnnot.c_str(), i_annot, drLUTAnnotations[i_name][i_annot]);
    }
    ::setSetting(settings, "array_lookup_tables." + name + ".PVLookupTable.IndexedLookup", 1);
    ::setSetting(settings, "array_lookup_tables." + name + ".PVLookupTable.NumberOfTableValues", 3);
  }
}

//-----------------------------------------------------------------------------
void lqLidarViewManager::updateInterfaceLayout(InterfaceModes mode)
{
  static bool firstCall = true;
  auto& intern = *this->Internals;

  if (firstCall)
  {
    firstCall = false;
  }
  else
  {
    // Save layout for the previous mode (it is not persistent with app closed)
    intern.saveModeState();
  }
  intern.setInterfaceMode(mode);

  intern.updateMenusLayout();
  intern.updateToolBarsLayout();
  intern.updateDockWidgetsLayout();
  intern.updateMultiViewWidgetVisibility();
  intern.updateShowScalarsBarsSettings();
  intern.updateLidarPlayerSpeedMode(mode);

  // Restore layout for the current mode
  intern.restoreModeState();

  // Update associated buttons
  Q_EMIT this->interfaceLayoutUpdated(mode);

  // Save current mode in settings
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->SetSetting(".settings.GeneralSettings.InterfaceMode", mode);
}

//-----------------------------------------------------------------------------
void lqLidarViewManager::restoreSavedInterfaceLayout()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  // Set default interface mode
  unsigned int interfaceMode = InterfaceModes::LIDAR_VIEWER;
  if (vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry())
  {
    // If we are testing lidarview default to ADVANCED_MODE
    interfaceMode = InterfaceModes::ADVANCED_MODE;
  }
  else
  {
    interfaceMode =
      settings->GetSettingAsInt(".settings.GeneralSettings.InterfaceMode", interfaceMode);
  }

  this->updateInterfaceLayout(static_cast<InterfaceModes>(interfaceMode));
}
