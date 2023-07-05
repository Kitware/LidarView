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

import lidarviewcore.kiwiviewerExporter as kiwiviewerExporter
import lidarview.gridAdjustmentDialog
import lidarview.planefit as planefit
import lidarview.simple as lvsmp

from PythonQt.paraview import lqCropReturnsDialog, lqSelectFramesDialog

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
      # WIP This can be removed through Statusbar creation and python wrappings in lqLidarViewManager
      # Fields that Can be overriden to show some statuses
      self.filenameLabel           = QtGui.QLabel()
      self.sensorInformationLabel  = QtGui.QLabel()
      self.positionPacketInfoLabel = QtGui.QLabel()
      statusBar = getMainWindow().statusBar()
      statusBar.addWidget(self.filenameLabel)
      statusBar.addWidget(self.sensorInformationLabel)
      statusBar.addWidget(self.positionPacketInfoLabel)

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

def getDefaultSaveFileName(extension, suffix='', frameId=None, baseName="Frame"):

    sensor = getSensor()
    reader = getReader()

    # Use current datetime and Interpreter DefaultRecordFileName
    if sensor and sensor.Interpreter:
        sensor.Interpreter.UpdatePipelineInformation()
        return '%s.%s' % (sensor.Interpreter.GetProperty("DefaultRecordFileName")[0], extension)

    # Use PCAP Basename
    if reader:
        filename = reader.FileName
        filename = filename[0] if isinstance(filename, paraview.servermanager.FileNameProperty) else filename # WIP FIX ?
        basename =  os.path.splitext(os.path.basename(filename))[0]
        if frameId is not None:
            suffix = '%s(%s%04d)' % (suffix, baseName, frameId)
        return '%s%s.%s' % (basename, suffix, extension)

    else:
      return "filename"

# Main API
def UpdateApplogicCommon(lidar):
  # WIP ACTUALLY THINK ABOUT always enabled ok, just apply settings on current lidar actually needed
  # Overall on what buttons are on-off when there is data or not

  onCropReturns(False) # Dont show the dialog just restore settings

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

    # TR Disabled in Sensor mode
    app.trailingFramesSpinBox.enabled = True

    # Labels
    LidarPort = sensor.GetClientSideObject().GetListeningPort()
    app.filenameLabel.setText('Live sensor stream (Port:'+str(LidarPort)+')' )
    app.positionPacketInfoLabel.setText('')

    enableSaveActions() # WIP UNSURE

    updateUIwithNewLidar()

    rep = smp.Show(sensor)
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

    # TR
    app.trailingFramesSpinBox.enabled = True
    onTrailingFramesChanged(app.trailingFramesSpinBox.value)

    filename = reader.FileName
    displayableFilename = os.path.basename(filename)
    # shorten the name to display because the status bar gives a lower bound to main window width
    shortDisplayableFilename = (displayableFilename[:59] + '...' + displayableFilename[-58:]) if len(displayableFilename) > 120 else displayableFilename
    app.filenameLabel.setText('File: %s' % shortDisplayableFilename)
    app.filenameLabel.setToolTip('File: %s' % displayableFilename)

    app.positionPacketInfoLabel.setText('') # will be updated later if possible

    enableSaveActions()

    getAnimationScene().UpdateAnimationUsingDataTimeSteps()

    smp.SetActiveView(smp.GetActiveView())

    showSourceInSpreadSheet(getTrailingFrame())

    updateUIwithNewLidar()

def rotateCSVFile(filename):

    # read the csv file, move the last 3 columns to the
    # front, and then overwrite the file with the result
    csvFile = open(filename, 'rt')
    reader = csv.reader(csvFile, quoting=csv.QUOTE_NONNUMERIC)
    rows = [row[-3:] + row[:-3] for row in reader]
    csvFile.close()

    writer = csv.writer(open(filename, 'wt'), quoting=csv.QUOTE_NONNUMERIC, delimiter=',', lineterminator = '\n')
    writer.writerows(rows)


def savePositionCSV(filename):
    w = smp.CreateWriter(filename, getPosition())
    w.Precision = 16
    w.FieldAssociation = 'Point Data'
    w.UpdatePipeline()
    smp.Delete(w)

def saveCSVCurrentFrame(filename):
    w = smp.CreateWriter(filename, getLidar())
    w.Precision = 16
    w.FieldAssociation = 'Point Data'
    w.UpdatePipeline()
    smp.Delete(w)
    rotateCSVFile(filename)

def saveCSVCurrentFrameSelection(filename):
    source = smp.GetActiveSource()
    extractSelection = smp.ExtractSelection(Input = source)
    w = smp.CreateWriter(filename, extractSelection)
    w.Precision = 16
    w.FieldAssociation = 'Point Data'
    w.UpdatePipeline()
    smp.Delete(w)
    rotateCSVFile(filename)

def saveFrameRange(filename, frameStart, frameStop, saveFunction):
    timesteps = range(frameStart, frameStop+1)
    saveFunction(filename, timesteps)


def saveCSV(filename, timesteps):

    tempDir = kiwiviewerExporter.tempfile.mkdtemp()
    basenameWithoutExtension = os.path.splitext(os.path.basename(filename))[0]
    outDir = os.path.join(tempDir, basenameWithoutExtension)
    filenameTemplate = os.path.join(outDir, basenameWithoutExtension + '_%04d.csv')
    os.makedirs(outDir)

    writer = smp.CreateWriter('tmp.csv', getLidar())
    writer.FieldAssociation = 'Point Data'
    writer.Precision = 16

    for i in timesteps:
        getAnimationScene().AnimationTime = getLidar().TimestepValues[i]
        writer.FileName = filenameTemplate % i
        writer.UpdatePipeline()
        rotateCSVFile(writer.FileName)

    smp.Delete(writer)

    kiwiviewerExporter.zipDir(outDir, filename)
    kiwiviewerExporter.shutil.rmtree(tempDir)

def getSaveFileName(title, extension, defaultFileName=None):

    settings = getPVSettings()
    defaultDir = settings.value('LidarPlugin/OpenData/DefaultDir', PythonQt.QtCore.QDir.homePath())
    defaultFileName = defaultDir if not defaultFileName else os.path.join(defaultDir, defaultFileName)

    filters = '%s (*.%s)' % (extension, extension)
    selectedFilter = '%s (*.%s)' % (extension, extension)
    fileName = QtGui.QFileDialog.getSaveFileName(getMainWindow(), title,
                        defaultFileName, filters, selectedFilter, 0)

    if fileName:
        settings.setValue('LidarPlugin/OpenData/DefaultDir', PythonQt.QtCore.QFileInfo(fileName).absoluteDir().absolutePath())
        return fileName

# Action related Logic
def getFrameSelectionFromUser(frameStrideVisibility=False, framePackVisibility=False, frameTransformVisibility=False):
    class FrameOptions(object):
        pass

    dialog = PythonQt.paraview.lqSelectFramesDialog(getMainWindow())
    dialog.frameMinimum = 0
    if getReader() is None:
        dialog.frameMaximum = 0
    elif getReader().GetClientSideObject().GetShowFirstAndLastFrame():
        dialog.frameMaximum = getReader().GetClientSideObject().GetNumberOfFrames() - 1
    else:
        dialog.frameMaximum = getReader().GetClientSideObject().GetNumberOfFrames() - 3
    dialog.frameStrideVisibility = frameStrideVisibility
    dialog.framePackVisibility = framePackVisibility
    dialog.frameTransformVisibility = frameTransformVisibility
    dialog.restoreState()

    if not dialog.exec_():
        return None

    frameOptions = FrameOptions()
    frameOptions.mode = dialog.frameMode
    frameOptions.start = dialog.frameStart
    frameOptions.stop = dialog.frameStop
    frameOptions.stride = dialog.frameStride
    frameOptions.pack = dialog.framePack
    frameOptions.transform = dialog.frameTransform

    dialog.setParent(None)

    return frameOptions

def onSavePosition():
    fileName = getSaveFileName('Save CSV', 'csv', getDefaultSaveFileName('csv', '-position'))
    if fileName:
        savePositionCSV(fileName)

def onSavePCAP():
    # It is not possible to save as PCAP during stream as we need frame numbers
    if getSensor():
        QtGui.QMessageBox.information(getMainWindow(),
                                      'Save As PCAP not available during stream',
                                      'Saving as PCAP is not possible during lidar stream mode. '
                                      'Please use the "Record" tool, and open the resulting pcap offline to process it.')
        return

    frameOptions = getFrameSelectionFromUser(frameTransformVisibility=False)
    if frameOptions is None:
        return

    if frameOptions.mode == lqSelectFramesDialog.CURRENT_FRAME:
        frameOptions.start = getFrameFromAnimationTime(getAnimationScene().AnimationTime)
        frameOptions.stop = frameOptions.start
        defaultFileName = getDefaultSaveFileName('pcap', frameId=frameOptions.start)
    elif frameOptions.mode == lqSelectFramesDialog.ALL_FRAMES:
        frameOptions.start = 0
        frameOptions.stop = 0 if getReader() is None else getReader().GetClientSideObject().GetNumberOfFrames() - 1
        defaultFileName = getDefaultSaveFileName('pcap')
    else:
        defaultFileName = getDefaultSaveFileName('pcap', suffix=' (Frame %d to %d)' % (frameOptions.start, frameOptions.stop))

    fileName = getSaveFileName('Save PCAP', 'pcap', defaultFileName)
    if not fileName:
        return
    PythonQt.paraview.lqLidarViewManager.saveFramesToPCAP(getReader().SMProxy, frameOptions.start, frameOptions.stop, fileName)

def getFrameFromAnimationTime(time):
    if not getReader():
        return -1

    index = bisect.bisect_right(getAnimationScene().TimeKeeper.TimestepValues, time)
    if index > 0:
        previousTime = getAnimationScene().TimeKeeper.TimestepValues[index - 1]
        nextTime     = getAnimationScene().TimeKeeper.TimestepValues[index]
        index = index - 1 if (abs(previousTime - time) < abs(nextTime - time)) else index
    return index

def exportToDirectory(outDir, timesteps):

    filenames = []

    alg = smp.GetActiveSource().GetClientSideObject()

    writer = vtk.vtkXMLPolyDataWriter()
    writer.SetDataModeToAppended()
    writer.EncodeAppendedDataOff()
    writer.SetCompressorTypeToZLib()

    for t in timesteps:

        filename = 'frame_%04d.vtp' % t
        filenames.append(filename)

        getAnimationScene().AnimationTime = t
        polyData = vtk.vtkPolyData()
        polyData.ShallowCopy(alg.GetOutput())

        writer.SetInputData(polyData)
        writer.SetFileName(os.path.join(outDir, filename))
        writer.Update()

    return filenames


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

    # Reset Labels
    app.filenameLabel.setText('')
    app.sensorInformationLabel.setText('')
    app.positionPacketInfoLabel.setText('')
    getMainWindow().statusBar().showMessage('')

    # Disable Actions
    disableSaveActions()

# Generic Helpers
def _setSaveActionsEnabled(enabled):
    for action in ('SavePCAP', 'Close', 'CropReturns'):
        app.actions['action'+action].setEnabled(enabled)
    getMainWindow().findChild('QMenu', 'menuSaveAs').enabled = enabled


def enableSaveActions():
    _setSaveActionsEnabled(True)
    if getPosition():
        app.actions['actionSavePositionCSV'].setEnabled(True)


def disableSaveActions():
    _setSaveActionsEnabled(False)
    app.actions['actionSavePositionCSV'].setEnabled(False)


def unloadData():
    for k, src in smp.GetSources().items():
        if src != app.grid and src != smp.FindSource("RPM"):
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

def getTrailingFrame(index = -1):
  return paraview.servermanager._getPyProxy(PythonQt.paraview.lqSensorListWidget.getTrailingFrame(index))

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

def onCropReturns(show = True):
    dialog = lqCropReturnsDialog(getMainWindow())

    cropEnabled = False
    cropOutside = False
    firstCorner = QtGui.QVector3D()
    secondCorner = QtGui.QVector3D()

    lidarInterpreter = getLidarPacketInterpreter()

    # Retrieve current values to fill the UI
    if lidarInterpreter:
        cropEnabled = lidarInterpreter.CropMode != 'None'
        cropOutside = lidarInterpreter.CropOutside
        firstCorner = QtGui.QVector3D(lidarInterpreter.CropRegion[0], lidarInterpreter.CropRegion[2], lidarInterpreter.CropRegion[4])
        secondCorner = QtGui.QVector3D(lidarInterpreter.CropRegion[1], lidarInterpreter.CropRegion[3], lidarInterpreter.CropRegion[5])

    #show the dialog box
    if show:
        dialog.cropOutside = cropOutside
        dialog.firstCorner = firstCorner
        dialog.secondCorner = secondCorner
        dialog.croppingEnabled = cropEnabled
        # Enforce the call to dialog.croppingEnabled."onChanged" even if dialog.croppingEnabled == cropEnabled
        dialog.croppingEnabled = not dialog.croppingEnabled
        dialog.croppingEnabled = not dialog.croppingEnabled

        # update the dialog configuration
        dialog.UpdateDialogWithCurrentSetting()

        if not dialog.exec_():
            return

    if lidarInterpreter:
        lidarInterpreter.CropOutside = dialog.cropOutside
        dialogCropMode = ['None', 'Cartesian', 'Spherical']
        lidarInterpreter.CropMode = dialogCropMode[dialog.GetCropMode()]
        p1 = dialog.firstCorner
        p2 = dialog.secondCorner
        lidarInterpreter.CropRegion = [p1.x(), p2.x(), p1.y(), p2.y(), p1.z(), p2.z()]
        if show:
            smp.Render()


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
    createRPMBehaviour()

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

def onTrailingFramesChanged(number):
  # WIP sensorListWidget must provide an API / assume responsibility for this
  tr = getTrailingFrame()
  if tr:
    tr.NumberOfTrailingFrames = number
    smp.Render()

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

def toggleRPM():
    rpm = smp.FindSource("RPM")
    if rpm:
        if app.actions['actionShowRPM'].isChecked():
            smp.Show(rpm)
        else:
            smp.Hide(rpm)
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
    app.actions['actionSavePositionCSV'].connect('triggered()', onSavePosition)
    app.actions['actionSavePCAP'].connect('triggered()', onSavePCAP)
    app.actions['actionGrid_Properties'].connect('triggered()', onGridProperties)
    app.actions['actionCropReturns'].connect('triggered()', onCropReturns)
    app.actions['actionShowPosition'].connect('triggered()', ShowPosition)
    app.actions['actionShowRPM'].connect('triggered()', toggleRPM)

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

    # Get playback speed control toolbar
    timeToolBar = mW.findChild('QToolBar','Player Control')

    # Trailing Frame Spinbox
    spinBoxLabel = QtGui.QLabel('TF:')
    spinBoxLabel.toolTip = "Number of trailing frames"
    timeToolBar.addWidget(spinBoxLabel)
    spinBox = QtGui.QSpinBox()
    spinBox.toolTip = "Number of trailing frames"
    spinBox.setMinimum(0)
    spinBox.setMaximum(100)
    spinBox.connect('valueChanged(int)', onTrailingFramesChanged)
    app.trailingFramesSpinBox = spinBox
    timeToolBar.addWidget(app.trailingFramesSpinBox)

    displayWidget = getMainWindow().findChild('lqColorToolbar').findChild('pqDisplayColorWidget')
    displayWidget.connect('arraySelectionChanged ()',adjustScalarBarRangeLabelFormat)
    app.actions['actionScalarBarVisibility'].connect('triggered()',adjustScalarBarRangeLabelFormat)


def createRPMBehaviour():
    # create and customize a label to display the rpm
    rpm = smp.Text(guiName="RPM", Text="No RPM/FPS")
    representation = smp.GetRepresentation(rpm)
    representation.FontSize = 16
    representation.Color = [1,1,0]
    # create an python animation cue to update the rpm value in the label
    PythonAnimationCue1 = smp.PythonAnimationCue()
    PythonAnimationCue1.Script= """
import paraview.simple as smp
import lidarview.applogic as lv
def start_cue(self):
    pass

def tick(self):
    rpm = smp.FindSource("RPM")
    lidar = lv.getLidar()
    if (lidar):
      valrpm  = int(lidar.Interpreter.GetClientSideObject().GetRpm())
      valfps = int(lidar.Interpreter.GetClientSideObject().GetFrequency())
      if  (valrpm):
        rpm.Text = f"{str(valrpm)} RPM"
      elif(valfps):
        rpm.Text = f"{str(valfps)} FPS"
      else:
        rpm.Text = "No RPM/FPS Info"

def end_cue(self):
    pass
"""
    smp.GetAnimationScene().Cues.append(PythonAnimationCue1)
    # force to be consistant with the UI
    toggleRPM()
    smp.SetActiveSource(None)

# Status Bar helper
def updateUIwithNewLidar():
    lidar = getLidar()
    if lidar:
        app.sensorInformationLabel.setText(lidar.GetClientSideObject().GetSensorInformation(True))
        app.sensorInformationLabel.setToolTip(lidar.GetClientSideObject().GetSensorInformation())
    smp.Render()
