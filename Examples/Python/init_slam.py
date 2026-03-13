"""
Instantiate a SLAM Online filter on a LiDAR source.
This sets a nice default display settings and general parameters.
"""
from paraview.simple import *

# find lidar source and render view
renderView = GetActiveViewOrCreate('RenderView')
data = GetActiveSource()

# create a new 'SLAM (online)'
slam = SLAMonline(PointCloud=data,
                  Calibration=OutputPort(data,1))

# Default arrays in case the automatic detection is disabled
slam.Timearray = ['POINTS', 'adjustedtime']
slam.Intensityarray = ['POINTS', 'intensity']
slam.Laserringidarray = ['POINTS', 'laser_id']
slam.Elevationanglescalibration = ['ROWS', 'verticalAngles']

slam.Keypointsextractor = 'Spinning Sensor Keypoint Extractor'

slam.Keypointsmapsupdatestep = 10
slam.Outputcurrentkeypoints = 0
slam.Verbositylevel = '3) 2 + Sub-problems processing duration'
slam.Numberofthreads = 8

################################################################################
#  Current transformed frame (port 0)
################################################################################

# show data in view
slamDisplay_0 = Show(slam, renderView)
slamDisplay_0.Representation = 'Surface'

################################################################################
#  Trajectory (port 1)
################################################################################

# show data in view
slamDisplay_1 = Show(OutputPort(slam, 1), renderView)
slamDisplay_1.Representation = 'Surface'

################################################################################
#  Edge map (port 2)
################################################################################

# show data in view
slamDisplay_2 = Show(OutputPort(slam, 2), renderView)
slamDisplay_2.Representation = 'Surface'

################################################################################
#  Planes map (port 3)
################################################################################

# show data in view
slamDisplay_3 = Show(OutputPort(slam, 3), renderView)
slamDisplay_3.Representation = 'Surface'

################################################################################
#  Blobs map (port 4)
################################################################################

# show data in view
slamDisplay_4 = Show(OutputPort(slam, 4), renderView)
slamDisplay_4.Representation = 'Surface'

################################################################################
#  Edges keypoints (port 5)
################################################################################

# show data in view
slamDisplay_5 = Show(OutputPort(slam, 5), renderView)
slamDisplay_5.Representation = 'Surface'

################################################################################
#  Planes keypoints (port 6)
################################################################################

# show data in view
slamDisplay_6 = Show(OutputPort(slam, 6), renderView)
slamDisplay_6.Representation = 'Surface'

################################################################################
#  Blobs keypoints (port 7)
################################################################################

# show data in view
slamDisplay_7 = Show(OutputPort(slam, 7), renderView)
slamDisplay_7.Representation = 'Surface'

################################################################################
# Update style
################################################################################

# update the view to ensure updated data information
renderView.Update()

# Current transformed frame

ColorBy(slamDisplay_0, ('POINTS', 'intensity'))
slamDisplay_0.RescaleTransferFunctionToDataRange(True, False)
slamDisplay_0.SetScalarBarVisibility(renderView, False)

intensityLUT = GetColorTransferFunction('intensity')
intensityPWF = GetOpacityTransferFunction('intensity')

# Trajectory

ColorBy(slamDisplay_1, ('POINTS', 'Covariance', 'Magnitude'))
slamDisplay_1.RescaleTransferFunctionToDataRange(True, False)
slamDisplay_1.SetScalarBarVisibility(renderView, False)
slamDisplay_1.LineWidth = 4.0

covarianceLUT = GetColorTransferFunction('Covariance')
covariancePWF = GetOpacityTransferFunction('Covariance')
covarianceLUT.RescaleTransferFunction(0.0, 0.05)
covariancePWF.RescaleTransferFunction(0.0, 0.05)

# Current maps

ColorBy(slamDisplay_2, ('POINTS', 'intensity'))
slamDisplay_2.RescaleTransferFunctionToDataRange(True, False)
slamDisplay_2.SetScalarBarVisibility(renderView, True)
slamDisplay_2.PointSize = 4.0

ColorBy(slamDisplay_3, ('POINTS', 'intensity'))
slamDisplay_3.RescaleTransferFunctionToDataRange(True, False)
slamDisplay_3.SetScalarBarVisibility(renderView, True)
slamDisplay_3.PointSize = 3.0

ColorBy(slamDisplay_4, ('POINTS', 'intensity'))
slamDisplay_4.RescaleTransferFunctionToDataRange(True, False)
slamDisplay_4.SetScalarBarVisibility(renderView, True)
slamDisplay_4.PointSize = 3.0

# Current keypoints

slamDisplay_5.DiffuseColor = [1.0, 1.0, 0.0]
slamDisplay_5.PointSize = 8.0

slamDisplay_6.DiffuseColor = [0.3333333333333333, 0.6666666666666666, 1.0]
slamDisplay_6.PointSize = 5.0

slamDisplay_7.DiffuseColor = [0.3333333333333333, 0.6666666666666666, 0.0]
slamDisplay_7.PointSize = 4.0

# update the view to ensure updated data information
renderView.Update()
SetActiveSource(slam)
