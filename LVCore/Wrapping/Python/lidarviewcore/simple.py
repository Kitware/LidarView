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

import os
import paraview.simple as smp

import importlib

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
def _SetParamsOpenLidar(source, interpreterName, **params):
    """Internal method used by OpenPCAP and OpenSensorStream"""
    source.Interpreter = interpreterName
    for prop in ["Translate", "Rotate"]:
        if prop in params:
            source.Interpreter.SensorTransform.__setattr__(prop, params[prop])
            del params[prop]

    properties = source.ListProperties()
    for prop in properties:
        if prop in params:
            source.SetPropertyWithName(prop, params[prop])
    return source

# -----------------------------------------------------------------------------
def OpenPCAP(filename, calibration, interpreter, **params):
    """Open a .pcap file. Read the pcap data contained in the file located at filename.

    **Parameters**

        filename (str):
          Full path of the pcap file to read.

        calibration (str):
          Full path to the corresponding calibration file or, if the calibration
          is a default caibration, only the file name is required.
          (Automatically search in share/ folder)

        interpreter (str):
          The interpreter to use. Note their availability depends
          of interpreter plugins built.

    **Keyword Parameters (optional)**

        LidarPort (int):
          Specify on which port packets are read. By default all port are read.

        ShowFirstAndLastFrame (bool):
          Show first and last frame. It is common that the first and last frame
          are incomplete (not a full rotation or not a full sweep), thus it is more
          esthetically pleasing (and less unsettling) to hide them.

        UseRelativeStartTime (bool):
          This option interprets the pcap file considering its first time stamp
          to be zero. It can be used for instance when opening multiple sensors to have
          them starting at the same time.

        DetectFrameDropping (bool):
          Throw a warning to the user each time a frame is dropped

        Translate ([double, double, double]):
          Translate point cloud from origin to specified (x, y, z) coordinates.

        Rotate ([double, double, double]):
          Rotate point cloud with specified yaw, roll and pitch.
    """
    calibrationFile = _SearchCalibrationFiles(calibration)

    reader = smp.LidarReader(FileName=filename, CalibrationFile=calibrationFile)
    reader = _SetParamsOpenLidar(reader, interpreter, **params)

    # Update animation scene based on data timesteps loaded in pcap
    animationScene = smp.GetAnimationScene()
    animationScene.UpdateAnimationUsingDataTimeSteps()

    smp.Show(reader)
    return reader

# -----------------------------------------------------------------------------
def OpenSensorStream(calibration, interpreter, port=2368, **params):
    """Open a lidar sensor stream on a port.

    **Parameters**

        calibration (str):
          Full path to the corresponding calibration file or, if the calibration
          is a default caibration, only the file name is required.
          (Automatically search in share/ folder)

        interpreter (str):
          The interpreter to use. Note their availability depends
          of interpreter plugins built.

        port (optional int):
          Port on which to listen for packets. Default 2368.

    **Keyword Parameters (optional)**

        DetectFrameDropping (bool):
          Throw a warning to the user each time a frame is dropped

        Translate ([double, double, double]):
          Translate point cloud from origin to specified (x, y, z) coordinates.

        Rotate ([double, double, double]):
          Rotate point cloud with specified yaw, roll and pitch.
    """
    calibrationFile = _SearchCalibrationFiles(calibration)

    stream = smp.LidarStream(CalibrationFile=calibrationFile)
    stream = _SetParamsOpenLidar(stream, interpreter, **params)

    smp.Show(stream)
    stream.Start()
    return stream

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
