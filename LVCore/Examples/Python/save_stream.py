"""
Dummy example to open a stream, record, stop, it and pause the stream
"""

import lidarview.simple as lvsmp
import time

################################################################################
outputfile = "C:/Demo/output.pcap"
calibfile = "VLP-16.xml"
interpreter = "Velodyne Packet Interpreter"
listeningport = 2368 # UDP port where the data is transmitted, default is 2368
################################################################################

# Open the stream - More info available in Wrapping/Python/simple.py
# Setting the port and DetectFrameDropping are optional here
stream = lvsmp.OpenSensorStream(calibfile,
                                interpreter,
                                listeningport,
                                DetectFrameDropping=True)

# Start recording after a few seconds
time.sleep(3)
stream.RecordingFilename = outputfile
stream.StartRecording()
time.sleep(10)

# Stop Recording
stream.StopRecording()

# Pause visualisation ( recording would not be paused )
time.sleep(3)
stream.Stop()

# Start it again
time.sleep(3)
stream.Start()
