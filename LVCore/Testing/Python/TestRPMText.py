import sys
import os

import paraview.simple as smp
import lidarview.simple as lvsmp

from paraview.vtk.util.misc import vtkGetDataRoot, vtkGetTempDir
from paraview.vtk.test import Testing

def TestBaseline(view, baseline_file):
  Testing.VTK_TEMP_DIR = vtkGetTempDir()
  Testing.compareImage(view.GetRenderWindow(), baseline_file, threshold=10)
  Testing.interact()

data_dir = vtkGetDataRoot()
try:
  baseline_index = sys.argv.index('-B')+1
  baseline_path = sys.argv[baseline_index]
except:
  sys.stderr.write("Could not get baseline directory. Test failed.")
  exit(1)

reader = lvsmp.OpenPCAP(f"{data_dir}/Slam/VLP-16_slam_test_data.pcap", "VLP-16", "Velodyne")
view = smp.GetRenderView()
rpm = smp.RPMInfo(Input=reader) # Use label in python
smp.Show(rpm)

scene = smp.GetAnimationScene()

smp.Render()

# RPM no initialized yet
baseline_file = os.path.join(baseline_path, "TestRPMText1.png")
print("Testing TestRPMText1...")
TestBaseline(view, baseline_file)

# Go to frame 10
for i in range(0, 10):
  # RPM calculator needs previous frame to calculate current RPM
  scene.GoToNext()
  # scene.AnimationTime = reader.TimestepValues[10]

# With Initialized RPM
baseline_file = os.path.join(baseline_path, "TestRPMText2.png")
print("Testing TestRPMText2...")
TestBaseline(view, baseline_file)
