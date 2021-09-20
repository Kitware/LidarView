from paraview.simple import *
import numpy as np
import os
import time
import math
from vtk.numpy_interface import dataset_adapter as dsa
from vtkmodules.vtkCommonDataModel import vtkPolyData
import PythonQt
from PythonQt import QtCore, QtGui, QtUiTools

# -----------------------------------------------------------------------------------------------------
def ComputeRotation(rotAxis, rotAngle):
    cosa = np.cos(rotAngle)
    sina = np.sin(rotAngle)
    ux = np.array([[0., -rotAxis[2], rotAxis[1]], [rotAxis[2], 0., -rotAxis[0]], [-rotAxis[1], rotAxis[0], 0.]])
    return cosa * np.identity(3) + sina * ux + (1 - cosa) * np.outer(rotAxis, rotAxis)

def findQObjectByName(widgets, name):
    for w in widgets:
        if w.objectName == name:
            return w


def getMainWindow():
    return findQObjectByName(QtGui.QApplication.topLevelWidgets(), 'vvMainWindow')

def doSLAMRegister(lidarMode, initialMapsPrefix, pathPCAP, calibFile, interpreterName, initialPose):
    # Planes map file loading for display:
    planesMapFile = initialMapsPrefix + 'planes.vtp'
    # Edges map file loading for display:
    edgesMapFile = initialMapsPrefix + 'edges.vtp'

    if not os.path.exists(planesMapFile) or not os.path.exists(edgesMapFile):
        print("Abort SLAM register : {} or {} is missing".format(planesMapFile, edgesMapFile))
        return


    if lidarMode == 0 :
        # ----------------------------------------Set Stream Mode--------------------------------------------
        stream = FindSource("LidarStream1")
    elif lidarMode == 1:
        # ----------------------------------------Set Reader Mode--------------------------------------------
        
        # Check if pcap or calibration file can be found
        if not os.path.exists(pathPCAP) or not os.path.exists(calibFile):
            print("Abort SLAM register : {} or {} is missing".format(pathPCAP, calibFile))
            return

        stream = LidarReader(CalibrationFile = calibFile, FileName = pathPCAP)
        stream.Interpreter = interpreterName
        fr = FindSource("Frame")
        Show(fr, GetActiveView())
    else:
        return


    # --------------------------------------Clean--------------------------------------
    paraview.simple._DisableFirstRenderCameraReset()
    # Update animation scene
    animation = GetAnimationScene()
    animation.UpdateAnimationUsingDataTimeSteps()
    animation.Stop()

    slam = FindSource('SLAM')
    if slam:
        Delete(slam)
    tf = FindSource('TrailingFrame')
    if tf:
        Delete(tf)

    # Delete last vtpPlanesData and vtpEdgesData if exists
    pdReader1 = FindSource("XMLPolyDataReader1")
    if pdReader1:
        Delete(pdReader1)
    pdReader2 = FindSource("XMLPolyDataReader2")
    if pdReader2:
        Delete(pdReader2)

    view = GetActiveView()
    if view is None:
        view = CreateView("DemoView")
        view.Background = [0.0, 0.0, 0.0]
        view.Background2 = [0.0, 0.0, 0.2]
        view.UseGradientBackground = True

    RenameView("DemoView", view)
    SetActiveView(view)

    # --------------------------------Maps loading for display--------------------------------
    # create 2 'XML PolyData Reader'
    vtpPlanesData = XMLPolyDataReader(FileName=[planesMapFile])
    vtpPlanesData.PointArrayStatus = ['Intensity']

    vtpEdgesData = XMLPolyDataReader(FileName=[edgesMapFile])
    vtpEdgesData.PointArrayStatus = ['Intensity']

    # Colorize by intensity
    # Planes
    vtpPlanesDisplay = Show(vtpPlanesData, view)
    ColorBy(vtpPlanesDisplay, ('POINTS', 'Intensity'))
    vtpPlanesDisplay.RescaleTransferFunctionToDataRange(True, False)
    vtpPlanesDisplay.SetScalarBarVisibility(view, True)

    # Edges
    vtpEdgesDisplay = Show(vtpEdgesData, view)
    ColorBy(vtpEdgesDisplay, ('POINTS', 'Intensity'))
    vtpEdgesDisplay.RescaleTransferFunctionToDataRange(True, False)
    vtpEdgesDisplay.SetScalarBarVisibility(view, True)

    # Update view
    view.Update()

    # ----------------------Apply SLAM with indoor configuration-----------------------------
    # Create SLAM filter
    slam = SLAMonline(PointCloud = stream,
                      SensorDataFile = '',
                      Initialmapfilesprefix='',
                      Calibration = None)
    RenameSource("SLAM", slam)

    # Arrays (Hesai)
    slam.Autodetectinputarrays = 0
    slam.Timearray = ['POINTS', 'Timestamp']
    slam.Intensityarray = ['POINTS', 'Intensity']
    slam.Laserringidarray = ['POINTS', 'LaserID']

    # Initialization
    slam.Initialmapfilesprefix = initialMapsPrefix
    slam.InitialposeXYZ = initialPose[:3]
    slam.InitialposeRPY = initialPose[3:]

    # Keypoints extraction
    slam.Keypointsextractor = 'Spinning Sensor Keypoint Extractor'
    slam.Keypointsextractor.Mindistancetosensor = 1.0
    slam.Keypointsextractor.Minlaserbeamtosurfaceangle = 10.0
    slam.Keypointsextractor.Neighborhoodwidth = 8
    slam.Keypointsextractor.Planemaxsinusangle = 0.5
    slam.Keypointsextractor.Edgeminsinusangle = 0.86
    slam.Keypointsextractor.Edgeminsaliencydistance = 1.5
    slam.Keypointsextractor.Edgeminintensitygap = 50.
    slam.Keypointsextractor.Edgemindepthgap = 0.5

    # General
    slam.Undistortionmode = 'Disabled'
    slam.EgoMotionmode = 'Disabled'
    slam.Keypointsmapsupdatestep = 10
    slam.Numberofthreads = 4
    slam.Useblobs = False

    # Optimization
    slam.ICPOptimizationiterations = 100
    slam.LMoptimizationiterations = 50
    slam.Maxneighborsdistance = 3.

    # Edges
    slam.Edgenbofneighbors = 9
    slam.Edgenbminfilteredneighbors = 4
    slam.EdgePCAfactor = 5.0
    slam.Edgemaxmodelerror = 0.1

    # Planes
    slam.Planenbofneighbors = 7
    slam.PlanePCAfactor1 = 5.0
    slam.PlanePCAfactor2 = 10.0
    slam.Planemaxmodelerror = 0.1

    # Robustifier
    slam.Initsaturationdistance = 3.
    slam.Finalsaturationdistance = 0.1

    # Modify map parameters
    # This is useless because the map is not modified anyway
    slam.Keyframedistancethreshold = 0.0
    slam.Keyframeanglethreshold = 0.0
    slam.Edgesmapresolution = 0.1
    slam.Planesmapresolution = 0.1
    slam.Rollinggriddimension = 100
    slam.Rollinggridresolution = 5.

    frame = FindSource("Frame")
    # Hide everything except the trajectory to extract a pose
    SetActiveSource(stream)
    Hide(frame, view)
    Hide(OutputPort(slam, 0), view)
    Show(OutputPort(slam, 1), view)
    Hide(OutputPort(slam, 2), view)
    Hide(OutputPort(slam, 3), view)
    Hide(OutputPort(slam, 4), view)
    Hide(OutputPort(slam, 5), view)
    Hide(OutputPort(slam, 6), view)
    Hide(OutputPort(slam, 7), view)

    # Reset if some data have already been processed with not updated parameters
    slam.Resetstate()
    view.Update()

    # --------------------------------------Wait for a few frames--------------------------------------
    # /!\ WARNING : this part is used to copy live stream context but it has to be commented for real use
    # animation = GetAnimationScene()
    # animation.Stop()
    # animation.GoToNext()
    # animation.GoToNext()

    # -------------------Use the new pose to transform next input frames--------------------------------
    # Get pose information
    # Create pose transform
    transformMatrix = np.array([[1.0, 0.0, 0.0, 0.0,], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]])
    trajectory = dsa.WrapDataObject(slam.GetClientSideObject().GetOutputDataObject(1))
    # Get rotation
    poseRotAngleAxis = trajectory.PointData['Orientation(AxisAngle)'][0]
    transformMatrix[0:3, 0:3] = ComputeRotation(poseRotAngleAxis[:3], poseRotAngleAxis[3])
    # Get translation
    transformMatrix[0:3, 3] = trajectory.Points[0]

    print("Transform used : ")
    print(transformMatrix)

    # Delete SLAM
    Delete(slam)

    # Display the new input scanned points transformed by the registration pose computed
    frameDisplay = Show(frame, view)
    frameDisplay.UserTransform = transformMatrix.flatten().tolist()
    view.Update()


# -----------------------------------------Run Main script-----------------------------------------------
def run():
    # -------------------------------------SLAM Register Parameters--------------------------------------

    # Initial maps files prefix (PCD files) for SLAM init:
    initialMapsPrefix = "/home/jerome/Dev/data/hesai/slam/"
    # Initial pose (where the Lidar is approximately in the maps)
    initialPose = [0.0, 0.0, 0.0, 0.0, 0.0, 3.14]

    # ----------------------------------------PCAP params------------------------------------------------
    # Initial Pcap filename to use in lidar Reader mode
    initialPcapFilename = "/home/jerome/Dev/data/hesai/128_OPENROAD.pcap"

    # Velodyne
    # initialCalibFilename = os.path.join(os.getcwd(), "..", "share", "VLP-16.xml")
    # if not(os.path.isfile(initialCalibFilename)):
    #     initialCalibFilename = os.path.join(os.getcwd(), "share", "VLP-16.xml")
    # initialInterpreter = 'Velodyne Meta Interpreter'

    # Ouster
    # initialCalibFilename = "/home/julia/Documents/Demo_Hesai/15h26/os1-991909000651-2048x10.pcap.json"
    # initialInterpreter = 'Ouster Interpreter'

    # Hesai
    initialCalibFilename = "/home/jerome/Dev/data/hesai/correction_ file_Pandar128.csv"
    initialInterpreter = 'Hesai Packet Interpreter'

    # ------------------------------------Get inputs from dialog-----------------------------------------
    loader = QtUiTools.QUiLoader()
    uifile = QtCore.QFile(':/vvResources/registerDialog.ui')
    if uifile.open(uifile.ReadOnly):
        dialog = loader.load(uifile, getMainWindow())
        uifile.close()
        dialog.setModal(True)

        # init dialog values from defaults
        dialog.dataDirLineEdit.setText(initialMapsPrefix)
        dialog.pcapFilenameLineEdit.setText(initialPcapFilename)
        dialog.calibFilenameLineEdit.setText(initialCalibFilename)

        # iniit pose in dialog
        dialog.xSpinBox.value       = initialPose[0]
        dialog.ySpinBox.value       = initialPose[1]
        dialog.zSpinBox.value       = initialPose[2]
        dialog.rollSpinBox.value    = initialPose[3]
        dialog.pitchSpinBox.value   = initialPose[4]
        dialog.yawSpinBox.value     = initialPose[5]

        if dialog.exec_():
            dialogPose = [dialog.xSpinBox.value, dialog.ySpinBox.value, dialog.zSpinBox.value, dialog.rollSpinBox.value, dialog.pitchSpinBox.value, dialog.yawSpinBox.value]
            doSLAMRegister( dialog.lidarModeComboBox.currentIndex,
                            dialog.dataDirLineEdit.text,
                            dialog.pcapFilenameLineEdit.text,
                            dialog.calibFilenameLineEdit.text,
                            initialInterpreter,
                            dialogPose)
        else:
            print("Reject : quit register script")
