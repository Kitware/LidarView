'''

This file is a helper in the purpose of create new test data
1) It generates all vtp files (reference frames files) for the pcap you want
2) It creates the "frame_list.txt" file that contains the name of every vtp files
3) It creates the "reference-data.xml" file that contains the GetNetworkTimeToDataTime reference

** How to use this code **

Add this script to the python shell, and then call generate_ground_truth with adequate arguments

'''

from paraview import simple as sp
from lidarview import simple as lsmp
import os.path
from pathlib import Path

"""
  filename: PCAP filename of the one you want to create test reference data
  model : LiDAR Model of the pcap, used to search for calibration (e.g VLP-16)
  vendor: Name of the interpreter vendor (e.g Velodyne)
"""
def generate_ground_truth(filename, model, vendor):
  args = {"IgnoreZeroDistances":True, "IgnoreEmptyFrames":True, "EnableAdvancedArrays":True, "ShowPartialFrames":True }

  reader = lsmp.OpenPCAP(filename, model, vendor, **args)
  sp.Show(reader)
  reader.UpdatePipeline()
  reader.UpdatePipelineInformation()

  # Save all vtp files in the data
  name = Path(filename).stem
  outFolder = str(Path(filename).parent)
  saveVTPFolder = os.path.join(outFolder, "{}.vtp".format(name))
  sp.SaveData(saveVTPFolder, proxy=reader, Writetimestepsasfileseries=1)

  # Create the frames_list.txt files that contains the list of all vtp file name
  frames_list_filename = os.path.join(outFolder, "frames_list.txt")
  with open(frames_list_filename, "a") as f:
    f.truncate(0)
    i = 0
    name_i = name + "_" + str(i) + ".vtp"
    currentFile = os.path.join(outFolder, name_i)
    while os.path.isfile(currentFile):
      f.write(name_i + "\n")
      i = i + 1
      name_i = name + "_" + str(i) + ".vtp"
      currentFile = os.path.join(outFolder, name_i)


  # Write the NetworkTime reference in the file
  time = reader.GetProperty("NetworkTimeToDataTime")
  with open(outFolder + "/" + name + "-reference-data.xml", "a") as f2:
    f2.truncate(0)
    contents = "<referenceData referenceNetworkTimeToDataTime=\"" + str(time) \
      + "\" baselineFile=\"frames_list.txt\">"
    f2.write(contents + "\n")
    f2.write("</referenceData>")
