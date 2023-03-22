"""
Dummy example to open a stream, record, stop, it and pause the stream  
"""
from paraview.simple import *
import time 
################################################################################
outputfile = 'C:/Demo/output.pcap'
calibfile = 'C:/Demo/VLP-16.xml'
listeningport = 2368    # UDP port where the data is transmitted, default is 2368 
################################################################################

# Set up calibration config ( containing the equivalent of what we can set up in the calibration window, including the calibration file ) 
cfg = lv.getCalibrationConfig()
cfg.setCalibrationFile(calibfile)

# open the stream
lv.openSensorStreamWithConfig(cfg)

# Start recording after a few seconds 
time.sleep(3)
lv.recordFile(outputfile)
time.sleep(10)

# Stop Recording
lv.stopRecording()

# Pause visualisation ( recording would not be paused )
time.sleep(3)
lv.onPause()

# Start it again 
time.sleep(3)
lv.onPlay()
