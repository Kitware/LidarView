'''

This file is a helper in the purpose of create new test data
1) It generates all vtp files (reference frames files) for the pcap you want
2) It creates the "frame_list.txt" file that contains the name of every vtp files
3) It creates the "reference-data.xml" file that contains the GetNetworkTimeToDataTime reference

** How to use this code **

You need to update all the variables that starts by UPDATE

This code has to be copied and pasted in a programmable source in LidarView
To create a programmable source : "Sources" -> "Programmable source"
Or "ctrl-space" -> "Programmable filter"

'''

from paraview import simple as sp
import os.path


# Folder where the GroundTruth folder is ALREADY created
# example of a good path : LV-Source/Plugins/PluginName/Testing/Data/Sensor/GroundTruth/'
UPDATE_GroundTruthFolder = ''

# This sensor name is only used in the name of the vtp files
UPDATE_SensorName = ''

# filename and calibration of the pcap you want to create test reference data for
# example of a good path : LV-Source/Plugins/PluginName/Testing/Data/Sensor/file.pcap/'
UPDATE_pcapFile = ''
UPDATE_calibrationFileName = ''

# Specific interpreter Name
# By default, paraview set the first interpreter in alphabetical order
# which belongs to the proxy group LidarPacketInterpreter
# If you want
UPDATE_interpreterName = ''


# Initialize the reader with the pcap and calibration files
reader = sp.LidarReader()
reader.FileName = UPDATE_pcapFile
reader.CalibrationFile = UPDATE_calibrationFileName
if UPDATE_interpreterName != "":
  reader.Interpreter = UPDATE_interpreterName
reader.Interpreter.IgnoreZeroDistances = 1
reader.Interpreter.IgnoreEmptyFrames = 1
reader.Interpreter.EnableAdvancedArrays = 1
reader.ShowFirstAndLastFrame = 1
sp.Show(reader)
reader.UpdatePipeline()
reader.UpdatePipelineInformation()

# Save all vtp files in the data
saveVTPFolder = os.path.join(UPDATE_GroundTruthFolder, "{}.vtp".format(UPDATE_SensorName))
sp.SaveData(saveVTPFolder, proxy=reader, Writetimestepsasfileseries=1)

# Create the frames_list.txt files that contains the list of all vtp file name
frames_list_filename = os.path.join(UPDATE_GroundTruthFolder, "frames_list.txt")
with open(frames_list_filename, "a") as f:
  f.truncate(0)
  i = 0
  name_i = UPDATE_SensorName + "_" + str(i) + ".vtp"
  currentFile = os.path.join(UPDATE_GroundTruthFolder, name_i)
  while os.path.isfile(currentFile):
    f.write(name_i + "\n")
    i = i + 1
    name_i = UPDATE_SensorName + "_" + str(i) + ".vtp"
    currentFile = os.path.join(UPDATE_GroundTruthFolder, name_i)


# Write the NetworkTime reference in the file
time = reader.GetClientSideObject().GetNetworkTimeToDataTime()
with open(UPDATE_GroundTruthFolder + "/../" + UPDATE_SensorName + "-reference-data.xml", "a") as f2:
  f2.truncate(0)
  contents = "<referenceData referenceNetworkTimeToDataTime=\"" + str(time) + "\">"
  f2.write(contents + "\n")
  f2.write("</referenceData>")
