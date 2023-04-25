import os

import paraview.simple as smp
import lidarview.simple as lvsmp

################################################################################
inputfile = 'C:/Demo/Sample_VLP16.pcap'
outputfile = 'C:/Demo/Sample_VLP16_export_timely.csv'
calibfile = 'C:/Demo/VLP-16.xml'
interpreter = "Velodyne Meta Interpreter"

################################################################################
def CropAndSave(filename, timesteps):
    fileDir = os.path.dirname(filename)
    basenameWithoutExtension = os.path.splitext(os.path.basename(filename))[0]
    filenameTemplate = os.path.join(fileDir, basenameWithoutExtension + '_%04d.csv')
    filenameCroppedTemplate = os.path.join(fileDir, basenameWithoutExtension + 'Cropped_%04d.csv')

    lidarSource = smp.FindSource('LidarReader1')

    #set up filter for cropping
    trailingFrame1 = smp.FindSource('TrailingFrame1')
    threshold = smp.Threshold(registrationName='Threshold1', Input=trailingFrame1)
    threshold.Scalars = ['POINTS', 'distance_m']

    ### Note : the below would be replaced by threshold.ThresholdRange = [0.0, 10.0] in former Paraview Versions ( such as in VV 5.1 )
    threshold.LowerThreshold = 0.0
    threshold.UpperThreshold = 10.0
    threshold.ThresholdMethod = 'Between'

    # Loop over the timesteps of interest
    for i in timesteps:

        timestamp = lidarSource.TimestepValues[i]
        smp.GetAnimationScene().SetPropertyWithName('AnimationTime',timestamp)
        #note : this could alternatively be done with lv.getPlayerController().onSeekFrame(i)

        threshold.UpdateVTKObjects()
        smp.UpdatePipeline(timestamp)

        filenameFull = filenameTemplate % i
        filenameCropped = filenameCroppedTemplate % i

        smp.SaveData(filenameFull, proxy=trailingFrame1, PointDataArrays=['adjustedtime', 'azimuth', 'distance_m', 'intensity', 'laser_id', 'timestamp', 'vertical_angle'])
        smp.SaveData(filenameCropped, proxy=threshold, PointDataArrays=['adjustedtime', 'azimuth', 'distance_m', 'intensity', 'laser_id', 'timestamp', 'vertical_angle'])

################################################################################

# open pcap file
lvsmp.OpenPCAP(inputfile, calibfile, interpreter)

# select frames 10 to 20
timesteps = range(10, 20)
CropAndSave(outputfile,timesteps)


