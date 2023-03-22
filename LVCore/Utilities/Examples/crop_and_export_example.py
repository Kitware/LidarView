import sys, os, csv, time, datetime, math, bisect

import paraview
import paraview.simple as smp

from paraview.simple import *
from paraview import vtk

################################################################################
inputfile = 'C:/Demo/Sample_VLP16.pcap'
outputfile = 'C:/Demo/Sample_VLP16_export_timely.csv'
calibfile = 'C:/Demo/VLP-16.xml'

################################################################################
def CropAndSave(filename, timesteps):
    fileDir = os.path.dirname(filename)
    basenameWithoutExtension = os.path.splitext(os.path.basename(filename))[0]
    filenameTemplate = os.path.join(fileDir, basenameWithoutExtension + '_%04d.csv')
    filenameCroppedTemplate = os.path.join(fileDir, basenameWithoutExtension + 'Cropped_%04d.csv')
    
    lidarSource = FindSource('LidarReader1')

    #set up filter for cropping 
    trailingFrame1 = FindSource('TrailingFrame1')
    threshold = Threshold(registrationName='Threshold1', Input=trailingFrame1)
    threshold.Scalars = ['POINTS', 'distance_m']
    
    ### Note : the below would be replaced by threshold.ThresholdRange = [0.0, 10.0] in former Paraview Versions ( such as in VV 5.1 )
    threshold.LowerThreshold = 0.0
    threshold.UpperThreshold = 10.0
    threshold.ThresholdMethod = 'Between'

    # Loop over the timesteps of interest 
    for i in timesteps:
        
        timestamp = lidarSource.TimestepValues[i]
        lv.getAnimationScene().SetPropertyWithName('AnimationTime',timestamp)
        #note : this could alternatively be done with lv.getPlayerController().onSeekFrame(i)

        threshold.UpdateVTKObjects()
        UpdatePipeline(timestamp)
        
        filenameFull = filenameTemplate % i
        filenameCropped = filenameCroppedTemplate % i

        SaveData(filenameFull, proxy=trailingFrame1, PointDataArrays=['adjustedtime', 'azimuth', 'distance_m', 'intensity', 'laser_id', 'timestamp', 'vertical_angle'])
        SaveData(filenameCropped, proxy=threshold, PointDataArrays=['adjustedtime', 'azimuth', 'distance_m', 'intensity', 'laser_id', 'timestamp', 'vertical_angle'])

################################################################################

# open pcap file
lv.openPcap(inputfile,calibfile)

# select frames 10 to 20
timesteps = range(10, 20)
CropAndSave(outputfile,timesteps)


