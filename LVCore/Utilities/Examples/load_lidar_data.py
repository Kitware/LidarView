"""
Open a pre-set LiDAR recording using a specified calibration and interpreter. 
"""
from paraview.simple import *

################################################################################
file_name = '/home/user/data/lidar.pcap'
calibration = '/home/user/software/lidarview/build/install/share/calib.xml'
interpreter = 'My Interpreter'
################################################################################

# create a new 'Lidar Reader'
data = LidarReader(FileName=file_name, CalibrationFile=calibration)
data.Interpreter = interpreter
data.Interpreter.SensorTransform = 'Transform2'

# get animation scene
animationScene1 = GetAnimationScene()
# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# set active source
SetActiveSource(data)

# get color transfer function/color map for 'intensity'
intensityLUT = GetColorTransferFunction('intensity')
intensityLUT.RGBPoints = [0.0, 0.0, 0.0, 1.0, 40.0, 1.0, 1.0, 0.0, 100.0, 1.0, 0.0, 0.0]
intensityLUT.ColorSpace = 'HSV'
# get opacity transfer function/opacity map for 'intensity'
intensityPWF = GetOpacityTransferFunction('intensity')

# show data in view
dataDisplay = Show(data, renderView1)
dataDisplay.Representation = 'Surface'

# set scalar coloring
ColorBy(dataDisplay, ('POINTS', 'intensity'))
# rescale color and/or opacity maps used to include current data range
dataDisplay.RescaleTransferFunctionToDataRange(True, False)
# hide color bar/color legend
dataDisplay.SetScalarBarVisibility(renderView1, False)