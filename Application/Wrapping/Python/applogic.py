# Copyright 2013 Velodyne Acoustics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys, os, csv, time, datetime, math, bisect

import paraview
import paraview.simple as smp
from paraview import vtk

import PythonQt
from PythonQt import QtCore, QtGui

import lidarview.gridAdjustmentDialog
import lidarview.planefit as planefit
import lidarview.simple as lvsmp

# import the vtk wrapping of the Lidar Plugin
# this enable to get the specific vtkObject behind a proxy via GetClientSideObject()
# without this plugin, GetClientSideObject(), would return the first mother class known by paraview
import lidarview.modules.lvCommonCore
import lidarview.modules.lvIOCore
import lidarview.modules.lvIOGeolocation
import lidarview.modules.lvIONetwork
import lidarview.modules.lvIOLidar

class AppLogic(object):

    def __init__(self):
        pass

# Array Helper
def hasArrayName(sourceProxy, arrayName):
    '''
    Returns True if the data has non-zero points and has a point data
    attribute with the given arrayName.
    '''
    if not sourceProxy:
        return False

    info = sourceProxy.GetDataInformation().DataInformation
    if info.GetNumberOfPoints() == 0:
        return False

    # ~ info = info.GetAttributeInformation(0)
    # ~ for i in range(info.GetNumberOfArrays()):
        # ~ if info.GetArrayInformation(i).GetName() == arrayName:
            # ~ return True
    # ~ return False
    array = sourceProxy.PointData.GetArray(arrayName)
    if array:
        return True

# Action Related Logic
def planeFit():
    planefit.fitPlane(app.actions['actionSpreadsheet'])

# Main API
def UpdateApplogicCommon(lidar):
  # WIP ACTUALLY THINK ABOUT always enabled ok, just apply settings on current lidar actually needed
  # Overall on what buttons are on-off when there is data or not

  # Reset Scene Time # WIP TIME CONTROLLER API ?
  smp.GetActiveView().ViewTime = 0.0

# Used by lqLidarViewManager
def UpdateApplogicLidar(lidarProxyName, gpsProxyName):

    sensor = smp.FindSource(lidarProxyName) #WIP use getSensor() and getPosOr()
    if not sensor:
      return

    UpdateApplogicCommon(sensor)

    sensor.UpdatePipelineInformation()
    sensor.UpdatePipeline()

    enableSaveActions() # WIP UNSURE

    smp.Show(sensor)
    showSourceInSpreadSheet(sensor)
    smp.Render()

# Used by lqLidarViewManager
def UpdateApplogicReader(lidarName, posOrName): # WIP could explicit send Proxy using _getPyProxy(vtkSMProxy)

    reader = getReader()
    if not reader :
      return

    UpdateApplogicCommon(reader)

    reader.UpdatePipelineInformation()
    reader.UpdatePipeline()

    enableSaveActions()

    getAnimationScene().UpdateAnimationUsingDataTimeSteps()

    smp.SetActiveView(smp.GetActiveView())

def onClose():
    # Pause
    smp.GetAnimationScene().Stop()
    # Remove Lidar Related
    unloadData()
    getAnimationScene().AnimationTime = 0
    # Remove widgets
    smp.HideUnusedScalarBars()

    # Reset Camera
    lvsmp.ResetCameraToForwardView()

    # Disable Actions
    disableSaveActions()

# Generic Helpers
def _setSaveActionsEnabled(enabled):
    app.actions['actionClose'].setEnabled(enabled)
    getMainWindow().findChild('QMenu', 'menuSaveAs').enabled = enabled


def enableSaveActions():
    _setSaveActionsEnabled(True)


def disableSaveActions():
    _setSaveActionsEnabled(False)


def unloadData():
    for k, src in smp.GetSources().items():
        if src != app.grid:
            smp.Delete(src)

    clearSpreadSheetView()

def getReaderSource():
  return PythonQt.paraview.lqSensorListWidget.getActiveLidarSource()

def getReader(index = -1):
  return paraview.servermanager._getPyProxy(PythonQt.paraview.lqSensorListWidget.getReader(index))

#def getLidarNew():
#  return paraview.servermanager._getPyProxy(PythonQt.paraview.lqSensorListWidget.getLidar())

def getSensor(index = -1):
  return paraview.servermanager._getPyProxy(PythonQt.paraview.lqSensorListWidget.getSensor(index))

def getPosOrSource(index = -1):
  return paraview.servermanager._getPyProxy(PythonQt.paraview.lqSensorListWidget.getPosOrSource(index))

def getLidar(index = -1): # WIP TODO
    return getReader(index) or getSensor(index)

def getLidarPacketInterpreter(): # WIP Used in places where explicit lidar / current lidar is mixed
    lidar = getLidar()
    if lidar:
      return lidar.Interpreter
    return None

def getPosition():
    return getattr(app, 'position', None)

def getSpreadSheetViewProxy(): #WIP this is probably unreliable
    return smp.servermanager.ProxyManager().GetProxy("views", "main spreadsheet view")

def clearSpreadSheetView():
    view = getSpreadSheetViewProxy()
    if view is not None:
        view.Representations = []


def showSourceInSpreadSheet(source):
  if not source:
    return
  spreadSheetView = getSpreadSheetViewProxy()
  smp.Show(source, spreadSheetView)

  # Work around a bug where the 'Showing' combobox doesn't update.
  # Calling hide and show again will trigger the refresh.
  smp.Hide(source, spreadSheetView)
  smp.Show(source, spreadSheetView)

def createGrid():
    app.grid = smp.GridSource(guiName='Measurement Grid')

    # Reset to default if not persistent asked
    if (getPVSettings().value('LidarPlugin/grid/gridPropertiesPersist') != "true"):
      # Default Settings # WIP NEED TO INIT OTHER PROPS ?
      app.grid.GridNbTicks = 10
    else:
        # Restore grid properties
        lineWidth = getPVSettings().value('LidarPlugin/grid/LineWidth')
        if lineWidth :
            app.grid.LineWidth = int(lineWidth)

        gridNbTicks = getPVSettings().value('LidarPlugin/grid/GridNbTicks')
        if gridNbTicks :
            app.grid.GridNbTicks = int(gridNbTicks)

        if getPVSettings().value('LidarPlugin/grid/Normal'):
            normal_x = getPVSettings().value('LidarPlugin/grid/Normal')[0]
            normal_y = getPVSettings().value('LidarPlugin/grid/Normal')[1]
            normal_z = getPVSettings().value('LidarPlugin/grid/Normal')[2]
            app.grid.Normal = [float(normal_x), float(normal_y), float(normal_z)]

        if getPVSettings().value('LidarPlugin/grid/Origin'):
            origin_x = getPVSettings().value('LidarPlugin/grid/Origin')[0]
            origin_y = getPVSettings().value('LidarPlugin/grid/Origin')[1]
            origin_z = getPVSettings().value('LidarPlugin/grid/Origin')[2]
            app.grid.Origin = [float(origin_x), float(origin_y), float(origin_z)]

        scale = getPVSettings().value(getPVSettings().value('LidarPlugin/grid/Scale'))
        if scale :
            app.grid.Scale = float(scale)

        if getPVSettings().value('LidarPlugin/grid/gridColor') :
            r = getPVSettings().value('LidarPlugin/grid/gridColor')[0]
            g = getPVSettings().value('LidarPlugin/grid/gridColor')[1]
            b = getPVSettings().value('LidarPlugin/grid/gridColor')[2]
            app.grid.Color = [float(r), float(g), float(b)]

    rep = smp.Show(app.grid)
    rep.LineWidth = app.grid.LineWidth
    rep.DiffuseColor = app.grid.Color
    rep.Pickable = 0
    rep.Visibility = 0
    smp.SetActiveSource(None)

    app.grid.UpdatePipeline()
    smp.Show(app.grid)
    return app.grid

def getAnimationScene():
    '''This function is a workaround because paraview.simple.GetAnimationScene()
    has an issue where the returned proxy might not have its Cues property initialized'''
    for proxy in paraview.servermanager.ProxyManager().GetProxiesInGroup("animation").values():
        if proxy.GetXMLName() == 'AnimationScene' and len(proxy.Cues):
            return proxy

# Main function, Used by lqLidarViewManager
def start():

    global app
    app = AppLogic()

    smp._DisableFirstRenderCameraReset()
    smp.GetActiveView().LODThreshold = 1e100

    lvsmp.ResetCameraToForwardView()

    setupActions()
    disableSaveActions()
    hideColorByComponent()

    # Create Grid #WIP not perfect requires loaded plugin
    createGrid()


def findQObjectByName(widgets, name):
    for w in widgets:
        if w.objectName == name:
            return w


def getMainWindow():
    return findQObjectByName(QtGui.QApplication.topLevelWidgets(), 'LidarViewMainWindow')


def getPVApplicationCore():
    return PythonQt.paraview.pqPVApplicationCore.instance()


def getPVSettings():
    return getPVApplicationCore().settings()

def onGridProperties():
    if not app.grid:
      createGrid()
    if lidarview.gridAdjustmentDialog.showDialog(getMainWindow(), app):
        rep = smp.Show(app.grid)
        rep.LineWidth    = app.grid.LineWidth
        rep.DiffuseColor = app.grid.Color

        if(getPVSettings().value('LidarPlugin/grid/gridPropertiesPersist') == "true") :
            getPVSettings().setValue('LidarPlugin/grid/gridColor', app.grid.Color)
            getPVSettings().setValue('LidarPlugin/grid/LineWidth', app.grid.LineWidth)
            getPVSettings().setValue('LidarPlugin/grid/GridNbTicks', app.grid.GridNbTicks)
            getPVSettings().setValue('LidarPlugin/grid/Normal', app.grid.Normal)
            getPVSettings().setValue('LidarPlugin/grid/Origin', app.grid.Origin)
            getPVSettings().setValue('LidarPlugin/grid/Scale', app.grid.Scale)

        smp.Render()

def hideColorByComponent():
    getMainWindow().findChild('lqColorToolbar').findChild('pqDisplayColorWidget').findChildren('QComboBox')[1].hide()

def adjustScalarBarRangeLabelFormat():
    if not app.actions['actionScalarBarVisibility'].isChecked():
        return

    arrayName = getMainWindow().findChild('lqColorToolbar').findChild('pqDisplayColorWidget').findChild('QComboBox').currentText
    if arrayName != '' and hasArrayName(getReader(), arrayName):
        sb = smp.GetScalarBar(smp.GetLookupTableForArray(arrayName, []))
        sb.RangeLabelFormat = '%g'
        smp.Render()

def transformMode():
    reader = getReader()
    if not reader:
        return None
    if hasattr(reader.Interpreter, 'ApplyTransform') and reader.Interpreter.ApplyTransform:
      return 1 # absolute
    else:
      return 0 # raw

def setTransformMode(mode):
    # 0 - raw
    # 1 - absolute
    # 2 - relative # WIP what ?
    reader = getReader()

    if reader:
        reader.Interpreter.ApplyTransform = (mode > 0)

def geolocationChanged(setting):
    setTransformMode(setting)
    smp.Render(view=smp.GetActiveView())

def onToogleAdvancedGUI(updateSettings = True):
  """ Switch the GUI between advanced and classic mode"""
  # hide/show Sources menu
  menuSources = getMainWindow().findChild("QMenu", "menuSources").menuAction()
  menuSources.visible = not menuSources.visible
  # hide/show Filters menu
  menuFilters = getMainWindow().findChild("QMenu", "menuFilters").menuAction()
  menuFilters.visible = not menuFilters.visible
  # hide/show Advance menu
  menuAdvance = getMainWindow().findChild("QMenu", "menuAdvance").menuAction()
  menuAdvance.visible = not menuAdvance.visible
  # hide/show view decorator
  getMainWindow().centralWidget().toggleWidgetDecoration()
  # update the UserSettings
  if updateSettings:
    # booleans must be store as int
    newValue = int(not int(getPVSettings().value("LidarPlugin/AdvanceFeature/Enable", 0)))
    getPVSettings().setValue("LidarPlugin/AdvanceFeature/Enable", newValue)

def switchVisibility(Proxy):
    """ Invert the Proxy visibility int the current view """
    ProxyRep = smp.GetRepresentation(Proxy)
    ProxyRep.Visibility = not ProxyRep.Visibility

def ShowPosition():
    position = getPosOrSource()
    if position:
        switchVisibility(position)
        smp.Render()

# Setup Actions
def setupActions():

    mW = getMainWindow()
    actions = mW.findChildren('QAction')

    app.actions = {}

    for a in actions:
        app.actions[a.objectName] = a

    app.actions['actionAdvanceFeature'].connect('triggered()', onToogleAdvancedGUI)
    app.actions['actionPlaneFit'].connect('triggered()', planeFit)
    app.actions['actionClose'].connect('triggered()', onClose)
    app.actions['actionGrid_Properties'].connect('triggered()', onGridProperties)
    app.actions['actionShowPosition'].connect('triggered()', ShowPosition)

    # Restore action states from settings
    settings = getPVSettings()

    advanceMode = int(settings.value("LidarPlugin/AdvanceFeature/Enable", 0))
    if not advanceMode:
      app.actions['actionAdvanceFeature'].checked = False
      onToogleAdvancedGUI(updateSettings=False)

    # Setup and add the geolocation toolbar
    geolocationToolBar = mW.findChild('QToolBar', 'geolocationToolbar')

    # Creating and adding the geolocation label to the geolocation toolbar
    geolocationLabel = QtGui.QLabel('Frame Mapping: ')
    geolocationToolBar.addWidget(geolocationLabel)

    # Creating the geolocation combobox
    geolocationComboBox = QtGui.QComboBox()

    # Add the different entries
    # Currently, as Absolute and Relative Geolocation options are broken, disable them.
    geolocationComboBox.addItem('None (RAW Data)')
    geolocationComboBox.setItemData(0, "No mapping: Each frame is at the origin", 3)

    geolocationComboBox.addItem('Absolute Geolocation')
    geolocationComboBox.setItemData(1, "Use GPS geolocation to get each frame absolute location, the first frame is shown at origin", 3)
    geolocationComboBox.model().item(1).setEnabled(False)

    geolocationComboBox.addItem('Relative Geolocation')
    geolocationComboBox.setItemData(2, "Use GPS geolocation to get each frame absolute location, the current frame is shown at origin", 3)
    geolocationComboBox.model().item(2).setEnabled(False)

    geolocationComboBox.connect('currentIndexChanged(int)', geolocationChanged)
    geolocationToolBar.addWidget(geolocationComboBox)

    # Set default toolbar visibility
    geolocationToolBar.visible = False

    displayWidget = getMainWindow().findChild('lqColorToolbar').findChild('pqDisplayColorWidget')
    displayWidget.connect('arraySelectionChanged ()',adjustScalarBarRangeLabelFormat)
    app.actions['actionScalarBarVisibility'].connect('triggered()',adjustScalarBarRangeLabelFormat)
