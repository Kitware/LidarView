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

pandarXT32_test_datapcap = lvsmp.OpenPCAP(f"{data_dir}/PandarXT-32_test_data.pcap", "PandarXT-32", "Hesai")
smp.Hide(pandarXT32_test_datapcap)

view = smp.GetRenderView()
scene = smp.GetAnimationScene()

initialization = 4
window = 3
motionDetection = smp.MotionDetector(Input=pandarXT32_test_datapcap)
motionDetection.Initializationtime = initialization
motionDetection.Windowsize = window
motionDetectionDisplay = smp.Show(motionDetection)
smp.ColorBy(motionDetectionDisplay, ('POINTS', 'Motion_label'))
smp.Render()

# initialize motion detector
for i in range(0, initialization - 1):
  scene.GoToNext()

baseline_file = os.path.join(baseline_path, "TestMotionDetector1.png")
print("Testing motion detector initialization phase...")
TestBaseline(view, baseline_file)

# Detect motion
for i in range(0, 4):
  scene.GoToNext()

smp.Show(motionDetection)
baseline_file = os.path.join(baseline_path, "TestMotionDetector2.png")
print("Testing motion detector detection phase...")
TestBaseline(view, baseline_file)
