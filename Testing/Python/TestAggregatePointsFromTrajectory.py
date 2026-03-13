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

pcap = lvsmp.OpenPCAP(f"{data_dir}/Slam/VLP-16_slam_test_data.pcap", "VLP-16", "Velodyne")
poses = smp.TemporalTransformsReader(FileName=f"{data_dir}/Slam/trajectory.poses")
smp.Hide(pcap)

view = smp.GetRenderView()
scene = smp.GetAnimationScene()

lastFrame = 45
trajOffline = smp.AggregatePointsFromTrajectoryoffline(PointCloud=pcap, Trajectory=poses, AllFrames=0, FirstFrame=0, LastFrame=lastFrame)
smp.Show(trajOffline)
smp.Render()

baseline_file = os.path.join(baseline_path, "TestAggregatePointsFromTrajectory.png")
print("Testing AggregatePointsFromTrajectory offline...")
TestBaseline(view, baseline_file)

trajOnline = smp.AggregatePointsFromTrajectoryonline(PointCloud=pcap, Trajectory=poses)
smp.Hide(trajOffline)
smp.Show(trajOnline)
smp.Render()

for i in range(0, lastFrame):
  scene.GoToNext()

print("Testing AggregatePointsFromTrajectory online...")
TestBaseline(view, baseline_file)
