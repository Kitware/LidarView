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
lvsmp.ResetCameraToForwardView(view)
smp.Render()

baseline_file = os.path.join(baseline_path, "TestResetCameraToForwardView.png")
TestBaseline(view, baseline_file)

lvsmp.ResetCameraLidar(view)
smp.Render()

baseline_file = os.path.join(baseline_path, "TestResetCameraLidar.png")
TestBaseline(view, baseline_file)
