#==============================================================================
#
#  Program: LidarView
#  Module:  simple.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================

import csv, os, sys

import importlib
import paraview.simple as smp

from lidarview.modules.lvRemotingServerManager import *

import lidarviewcore.kiwiviewerExporter as kiwiviewerExporter

# -----------------------------------------------------------------------------
def LoadLidarViewPythonPlugins(pluginName = None):
    """Load a LidarView python plugin form the lidarviewplugins python module.

    **Parameters**

        pluginName (optional str):
          If specified, load a specific plugin from the lidarviewplugins module.
    """
    toLoad = "lidarviewplugins" + ("." + pluginName if pluginName else "")
    pluginFile = importlib.import_module(toLoad).__file__
    smp.LoadPlugin(pluginFile, remote=False, ns=globals())

# -----------------------------------------------------------------------------
def _SearchCalibrationFiles(calibration):
    """Internal method used by OpenPCAP and OpenSensorStream

    Search if current path exist and then search for file in share directory
    with calibration name specified.
    """
    if os.path.exists(calibration):
        return calibration
    currentPath = os.path.dirname(os.path.abspath(__file__))
    defaultCalibrationFile = os.path.join(currentPath,
                                          "../../../../share/" + calibration)
    if os.path.exists(defaultCalibrationFile):
        return defaultCalibrationFile
    raise ValueError(f"A valid calibration file must be specified.")

# -----------------------------------------------------------------------------
def _SetParamsOpenLidar(source, **params):
    """Internal method used by OpenPCAP and OpenSensorStream"""
    properties = source.ListProperties()
    for prop in properties:
        if prop in params:
            source.SetPropertyWithName(prop, params[prop])
    return source

# -----------------------------------------------------------------------------
def GetAvailableInterpreterList(proxy=None):
    """Using the interpreter manager, return a list of all interpreters available
    """
    if not proxy:
      controller = smp.servermanager.ParaViewPipelineController()
      proxy = smp.servermanager.misc.InterpretersManager()
      controller.InitializeProxy(proxy)

    nb = proxy.GetNumberOfInterpreters()
    return [proxy.GetInterpreterName(i) for i in range(0, nb)]

# -----------------------------------------------------------------------------
def CreateInterpreterProxy(name, filename=None, mode=0, interpreterType=0):
    """Create the appropriate proxy based on the provided parameters using the interpreter manager.

    **Parameters**

        name (str):
            Name of the interpreter. Use GetAvailableInterpreterList() to get the list.

        filename (str): (required if mode is stream)
            Full path to the file to read in reader mode.

        mode (int):
            - if 0 == reader mode. (default)
            - if 1 == stream mode.

        interpreterType (int):
            - if 0 == lidar only proxy. (default)
            - if 1 == create a proxy with lidar + pose (GNSS/GPS) information.
    """
    if mode == 0 and not filename:
        raise RuntimeError("A filename is needed in reader mode.")

    if mode == 1 and filename:
        raise RuntimeError("No filename is needed in stream mode.")

    controller = smp.servermanager.ParaViewPipelineController()
    proxy = smp.servermanager.misc.InterpretersManager()
    controller.PreInitializeProxy(proxy)
    proxy.Mode = mode
    controller.PostInitializeProxy(proxy)

    intrepreterList = GetAvailableInterpreterList(proxy)
    if name not in intrepreterList:
        raise RuntimeError(f"{name} is not a valid interpreter.")

    proxyName = proxy.GetLidarProxyName(name, interpreterType)
    if not proxyName or len(proxyName) == 0:
        raise RuntimeError("No proxy with this configuration was found for this interpreter")

    interpreterProxy = smp.servermanager.CreateProxy("sources", proxyName)
    pyname = smp.servermanager._make_name_valid(interpreterProxy.GetXMLLabel())
    if mode == 0:
        return eval(f"smp.{pyname}(FileName=\"{filename}\")")
    return eval(f"smp.{pyname}()")

# -----------------------------------------------------------------------------
def OpenPCAP(filename, lidarModel, interpreter, **params):
    """Open a .pcap file. Read the pcap data contained in the file located at filename.

    **Parameters**

        filename (str):
          Full path of the pcap file to read.

        lidarModel (str):
          Configure the lidar model if exist; otherwise, switch to legacy mode an
          specify the complete path to the corresponding calibration file.

        interpreter (str):
          The interpreter to use. Note their availability depends
          of interpreter plugins built.

    **Keyword Parameters (optional)**

        LidarPort (int):
          Specify on which port packets are read. By default all port are read.

        CalibrationFile (str):
          If specified, replace the default sensor calibration file.

        ShowPartialFrames (bool):
          Show first and last frame. It is common that the first and last frame
          are incomplete (not a full rotation or not a full sweep), thus it is more
          esthetically pleasing (and less unsettling) to hide them.

        DetectFrameDropping (bool):
          Throw a warning to the user each time a frame is dropped

        Translate ([double, double, double]):
          Translate point cloud from origin to specified (x, y, z) coordinates.

        Rotate ([double, double, double]):
          Rotate point cloud with specified yaw, roll and pitch.
    """
    reader = CreateInterpreterProxy(interpreter, filename=filename, mode=0)

    if hasattr(reader, 'LidarModel'):
      if lidarModel in reader.LidarModel.Available:
          reader.LidarModel = lidarModel
      else:
          reader.CalibrationFileName = _SearchCalibrationFiles(lidarModel)

    reader = _SetParamsOpenLidar(reader, **params)

    # Update animation scene based on data timesteps loaded in pcap
    animationScene = smp.GetAnimationScene()
    animationScene.UpdateAnimationUsingDataTimeSteps()

    smp.Show(reader)
    return reader

# -----------------------------------------------------------------------------
def OpenSensorStream(lidarModel, interpreter, **params):
    """Open a lidar sensor stream on a port.

    **Parameters**

        lidarModel (str):
          Configure the lidar model if it exists; otherwise, switch to legacy mode and
          specify the complete path to the corresponding calibration file.

        interpreter (str):
          The interpreter to use. Note their availability depends
          of interpreter plugins built.

    **Keyword Parameters (optional)**
        ListeningPort (int):
          Specify on which port packets are read.

        CalibrationFile (str):
          If specified, replace the default sensor calibration file.

        DetectFrameDropping (bool):
          Throw a warning to the user each time a frame is dropped

        Translate ([double, double, double]):
          Translate point cloud from origin to specified (x, y, z) coordinates.

        Rotate ([double, double, double]):
          Rotate point cloud with specified yaw, roll and pitch.
    """
    stream = CreateInterpreterProxy(interpreter, mode=1)

    if lidarModel in stream.LidarModel.Available:
        stream.LidarModel = lidarModel
    else:
        stream.CalibrationFileName = _SearchCalibrationFiles(lidarModel)

    stream = _SetParamsOpenLidar(stream, **params)

    smp.Show(stream)
    stream.Start()
    return stream

# -----------------------------------------------------------------------------
def RefreshStream(stream):
    """ Check if the stream needs to be refreshed and update it if necessary.
    Return True if the live source was refreshed, otherwise False.

    **Parameters**

        stream (vtkSMLidarStreamProxy):
          Stream to refresh, this should be an instance of `vtkSMLidarStreamProxy`.
    """
    if not stream:
        print("RefreshStream requires a valid stream.", file=sys.stderr)
        return False

    needUpdate = stream.SMProxy.DoesNeedsUpdate()
    if needUpdate:
      stream.SMProxy.MarkModified(stream.SMProxy)
      smp.RenderAllViews()
    return needUpdate

# -----------------------------------------------------------------------------
def RotateCSVFile(filename, nbOfColumns=3):
    """Change order of columns in csv file, this is usually done to bring the
    last 3 columns which are points coordinate in front. (For user lisibility)

    **Parameters**

        filename (str):
          Full path to the csv file to rotate.

        nbOfColumns (optional int):
          Number of columns to bring back in front. (default 3)
    """
    csvFile = open(filename, 'rt')
    reader = csv.reader(csvFile, quoting=csv.QUOTE_NONNUMERIC)
    rows = [row[-nbOfColumns:] + row[:-nbOfColumns] for row in reader]
    csvFile.close()

    writer = csv.writer(open(filename, 'wt'), quoting=csv.QUOTE_NONNUMERIC, delimiter=',', lineterminator = '\n')
    writer.writerows(rows)

# -----------------------------------------------------------------------------
def SaveCSV(source, filename, timesteps):
    """Save the source data in a zipped directory with all specified timesteps as csv.

    **Parameters**

        source (smp.Proxy):
          Paraview proxy that will be saved.

        filename (str):
          Full path to zipped directory

        timesteps (range):
          Save a csv for each timestep specified.

    Note: More generic smp.SaveData() from paraview.smp could be also used instead.
    """
    tempDir = kiwiviewerExporter.tempfile.mkdtemp()
    basenameWithoutExtension = os.path.splitext(os.path.basename(filename))[0]
    outDir = os.path.join(tempDir, basenameWithoutExtension)
    filenameTemplate = os.path.join(outDir, basenameWithoutExtension + '_%04d.csv')
    os.makedirs(outDir)

    writer = smp.CreateWriter('tmp.csv', Input=source)
    writer.FieldAssociation = 'Point Data'
    writer.Precision = 16

    for i in timesteps:
        timestamp = source.TimestepValues[i]
        smp.GetAnimationScene().AnimationTime = timestamp
        writer.FileName = filenameTemplate % i
        writer.UpdatePipeline(timestamp)
        RotateCSVFile(writer.FileName)

    smp.Delete(writer)

    kiwiviewerExporter.zipDir(outDir, filename)
    kiwiviewerExporter.shutil.rmtree(tempDir)

# -----------------------------------------------------------------------------
def ResetCameraLidar(view=None, distance=100):
    """Reset camera view to predefined viewpoint - 30 degrees behind the grid center.

    **Parameters**

        view (optional smp.RenderView):
          Renders the given view (default value is active view).

        distance (optional int):
          Specify the distance camera to viewpoint. (Default 100)
    """
    # Mirror of lqLidarCoreManager::onResetCameraLidar
    if not view:
      view = smp.GetRenderView()

    # See pqRenderView::resetViewDirection
    # Position at 30 degrees [0, -(squareRoot(3)/2)*dist, (1/2)*dist]
    view.CameraPosition = [0, -0.866025 * distance, (1.0 / 2.0) * distance]
    view.CameraFocalPoint = [0, 0, 0]
    view.CameraViewUp = [0, 0, 1]

    smp.Render(view)

# -----------------------------------------------------------------------------
def ResetCameraToForwardView(view=None):
    """Reset camera to forward view (default LidarView view)

    **Parameters**

        view (optional smp.RenderView):
          Renders the given view (default value is active view).
    """
    # Mirror of lqLidarCoreManager::onResetCameraToForwardView
    if not view:
      view = smp.GetRenderView()

    view.CameraPosition = [0.0, -72.0, 18.0]
    view.CameraFocalPoint = [0.0, 0.0, 0.0]
    view.CameraViewUp = [0.0, 0.27, 0.96]

    smp.Render(view)
