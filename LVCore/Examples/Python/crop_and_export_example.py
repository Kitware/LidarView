import os

import paraview.simple as smp
import lidarview.simple as lvsmp

################################################################################
inputfile = 'C:/Demo/Sample_VLP16.pcap'
outputfile = 'C:/Demo/Sample_VLP16_export_timely.csv'
lidarModel = 'VLP-16'
interpreter = "Velodyne"

################################################################################
def CropAndSave(reader, filename, timesteps):
    fileDir = os.path.dirname(filename)
    basenameWithoutExtension = os.path.splitext(os.path.basename(filename))[0]
    filenameTemplate = os.path.join(fileDir, basenameWithoutExtension + '_%04d.csv')
    filenameCroppedTemplate = os.path.join(fileDir, basenameWithoutExtension + 'Cropped_%04d.csv')

    #set up filter for cropping
    threshold = smp.Threshold(registrationName='Threshold1', Input=reader)
    threshold.Scalars = ['POINTS', 'distance_m']

    ### Note : the below would be replaced by threshold.ThresholdRange = [0.0, 10.0] in former Paraview Versions ( such as in VV 5.1 )
    threshold.LowerThreshold = 0.0
    threshold.UpperThreshold = 10.0
    threshold.ThresholdMethod = 'Between'

    # Loop over the timesteps of interest
    for i in timesteps:

        timestamp = reader.TimestepValues[i]
        smp.GetAnimationScene().AnimationTime = timestamp # or scene.GoToNext()

        threshold.UpdateVTKObjects()
        smp.UpdatePipeline(timestamp)

        filenameFull = filenameTemplate % i
        filenameCropped = filenameCroppedTemplate % i

        smp.SaveData(filenameFull, proxy=reader, PointDataArrays=['adjustedtime', 'azimuth', 'distance_m', 'intensity', 'laser_id', 'timestamp', 'vertical_angle'])
        smp.SaveData(filenameCropped, proxy=threshold, PointDataArrays=['adjustedtime', 'azimuth', 'distance_m', 'intensity', 'laser_id', 'timestamp', 'vertical_angle'])

################################################################################

if __name__ == "__main__":
# open pcap file
    reader = lvsmp.OpenPCAP(inputfile, lidarModel, interpreter)

    # select frames 10 to 20
    timesteps = range(10, 20)
    CropAndSave(reader, outputfile, timesteps)
