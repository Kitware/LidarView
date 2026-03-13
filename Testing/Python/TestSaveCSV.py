import sys
import os
import zipfile

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
smp.Render()

zipPath = f"{vtkGetTempDir()}/TestCSV"
timesteps = range(0, 10)
lvsmp.SaveCSV(reader, f"{zipPath}.zip", timesteps)

with zipfile.ZipFile(f"{zipPath}.zip", 'r') as zip_ref:
    zip_ref.extractall(vtkGetTempDir())

for i in [0, 8]:
    data = smp.OpenDataFile(f"{zipPath}/TestCSV_000{i}.csv")
    toPoint = smp.TableToPoints(Input=data, XColumn="Points_m_XYZ:0",  YColumn="Points_m_XYZ:1", ZColumn="Points_m_XYZ:2")
    smp.Show(toPoint)

smp.Render()

baseline_file = os.path.join(baseline_path, "TestSaveCSV.png")
TestBaseline(view, baseline_file)
