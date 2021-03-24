"""
This script is an example of how to detect the ground plane around the car in a
driving environment and what is over it.


Requirements:
-------------
This script uses the `DBSCANClustering` lidarview filter that requires lidarview
to be built with nanoflann enabled.
(use `cmake {source folder} -DENABLE_nanoflann=ON ...` when configuring cmake
for a Superbuild)


Overview:
---------
Considering a pointcloud (potentially with trailing frames):
- Select a subsample of the original point cloud in which we expect most of the
points to belong to the road (for example the area that is in front of the car
at the time of the data capture)
- Fit a plane on this subsample using RANSAC
- Align the original pointcloud with the fitted plane
- Select the candidate ground points in the whole pointcloud by selecting the
points that have `z` close to 0 (after aligning the whole pointcloud with the
plane)
- Cluster this subsample by distance between the points in order to select the
biggest cluster and consider it as the road (ie. the biggest connected area)
- Create a road mesh from this area in oder to classify the points from the
whole pointcloud according to their projection on the `z=0` plane. (Points that
have `z > 0 + tolerance` and have their projection inside the road will be
considered as obstacles.


Example of use:
---------------
- Adapt the values of sensor specific setting in the dedicated part of this
script.
- Open a lidar data file (`pcap` or `frame.vtp.series`)
- Optionnally (in order to aggregate several frames for more stable results)
    - open the file containing the sensor trajectory (usually a `.poses` file)
    - apply a temporal transform to have the lidar points in a "world" reference
    - apply a trailing frame filter to aggregate frames
- Select the point cloud in the pipeline interface
- Run this script


Result:
-------
Display a rotated point cloud with the plane "z = 0" corresponding to the
ground plane.
This point cloud is colored by label:
- blue: road
- red: obstacle, ie. object that is over the road
- green:other

"""
import paraview.simple as smp
import os


# ==============================================================================
# Sensor specific setting TO BE UPDATED BEFORE USE
# ==============================================================================

# Threshold values for restricted ground area selection
# The aim is to select a subsample of the original point cloud in which we
# expect most of the points to be part of the ground (e.g. the close area in
# front of the car)
# Range values for the distance to the sensor (select close points)
restrictedAreaDistanceRange = [1.5, 15]
# Range values for the azimuth of the points (select points in front of the car)
restrictedAreaAzimuthRange = [5000, 10000]


# ==============================================================================
# Pipeline
# ==============================================================================

# ----- Fetch inputs from current state ----------------------------------------
pointCloud = smp.GetActiveSource()
renderView = smp.GetActiveViewOrCreate('RenderView')

# ----- Select a restricted area for potential ground points ------------------
# Selection criteria are:
# - being close enough to the sensor
# - being in front of the vehicle
threshold1 = smp.Threshold(Input=pointCloud)
threshold1.Scalars = ['POINTS', 'distance_m']
threshold1.ThresholdRange = restrictedAreaDistanceRange
threshold1.UpdatePipeline()

threshold2 = smp.Threshold(Input=threshold1)
threshold2.Scalars = ['POINTS', 'azimuth']
threshold2.ThresholdRange = restrictedAreaAzimuthRange
threshold2.UpdatePipeline()

groundCandidate = smp.ExtractSurface(Input=threshold2)
groundCandidate.UpdatePipeline()

# Pass only points coordnates in order to make the computation faster
groundCandidate = smp.PassArrays(groundCandidate)
groundCandidate.PointDataArrays = []
groundCandidate.FieldDataArrays = []
groundCandidate.UpdatePipeline()


# ----- Fit a plane using ransac -----------------------------------------------
groundPointCloud = smp.RansacPlaneModel(Input=groundCandidate,
                                        MaxIteration=1000,
                                        Threshold=0.2,
                                        RatioInliersRequired=0.3,
                                        AlignOutput=0,
                                        TemporalAveraging=1,
                                        PreviousEstimationWeight=0.5,
                                        MaxTemporalAngleChange=20.0)
groundPointCloud.UpdatePipeline()


# Rotate the input point cloud to fit the ground to the `z = 0` plane ----------
# This will enable discriminating the points that belong to the ground  by their
# z coordinate
PF_rotateFitGround = smp.ProgrammableFilter(Input=[groundPointCloud, pointCloud],
                                            guiName='rotateFitGround')
PF_rotateFitGround.RequestInformationScript = ''
PF_rotateFitGround.RequestUpdateExtentScript = ''
PF_rotateFitGround.PythonPath = ''

PF_rotateFitGround.Script = """
import numpy as np
import vtk

input0 = inputs[0]
input1 = inputs[1]

nbarrays0 = input0.GetFieldData().GetNumberOfArrays()
input0isplane = any([input0.GetFieldData().GetArrayName(ii) == 'PlaneParam' for ii in range(nbarrays0)])

nbarrays1 = input1.GetFieldData().GetNumberOfArrays()
input1isplane = any([input1.GetFieldData().GetArrayName(ii) == 'PlaneParam' for ii in range(nbarrays1)])

if input0isplane:
    planeinput = input0
    pcinput = input1

elif input1isplane:
    planeinput = input1
    pcinput = input0

else:
    print("PlaneParam array does not exist in any input. Please check")
    return


planeparameters = planeinput.GetFieldData().GetArray('PlaneParam')[0]
a, b, c, d = planeparameters
n = [a, b, c]
z = [0, 0, 1]
nnorm = n / np.linalg.norm(n)
v = np.cross(nnorm, z)
theta = np.arccos(np.dot(nnorm, z)) # radians
theta = theta / np.pi * 180

transform = vtk.vtkTransform()
transform.Translate(0, 0, d / c)
transform.RotateWXYZ(theta, v)
transform.Update()

transformPD = vtk.vtkTransformPolyDataFilter()
transformPD.SetTransform(transform)
transformPD.SetInputData(pcinput.VTKObject)
transformPD.Update()

transformedInput = transformPD.GetOutput()

correctedZ = vtk.vtkFloatArray()
correctedZ.SetNumberOfComponents(1)
correctedZ.SetName('corrected_Z')

numPoints = transformedInput.GetNumberOfPoints()
for i in range(0, numPoints):
    coord = transformedInput.GetPoint(i)
    x, y, z = coord[:3]
    correctedZ.InsertNextValue(z)

    transformedInput.GetPointData().AddArray(correctedZ)
    output.ShallowCopy(transformedInput)
"""
PF_rotateFitGround.UpdatePipeline()
smp.RenameSource('Rotated pointcloud', PF_rotateFitGround)


# ----- Extract the road area (as a mesh) in the whole pointcloud ------------
# Filter points in the rotated point cloud based on the elevation (points with
# z close enough to  0 in the rotated pointcloud are considered as belonging to
# the ground)
# Then use clustering to filter out points that don't belong to the road:
# -> The biggest cluster in the ground plane is considered as the road
# And create a mesh from the resulting points

selectExtendedGround = smp.Threshold(Input=PF_rotateFitGround)
selectExtendedGround.Scalars = ['POINTS', 'corrected_Z']
selectExtendedGround.ThresholdRange = [-0.2, 0.2]
selectExtendedGround.UpdatePipeline()

extendedGround = smp.ExtractSurface(Input=selectExtendedGround)

flatGround = smp.PlaneProjector(PointCloud=extendedGround, Plane=None)
clusters = smp.DBSCANClustering(Input=flatGround, Epsilon=2, MinPts=20)

PF_selectBiggestCluster = smp.ProgrammableFilter(Input=clusters,
                                                 guiName='selectBiggestCluster')
PF_selectBiggestCluster.RequestInformationScript = ''
PF_selectBiggestCluster.RequestUpdateExtentScript = ''
PF_selectBiggestCluster.PythonPath = ''

PF_selectBiggestCluster.Script = """
import vtk
from vtk.util import numpy_support
import numpy as np

pdi = self.GetPolyDataInput()
pdo = self.GetPolyDataOutput()
clusters_vtk = pdi.GetPointData().GetArray('cluster')
clusters = numpy_support.vtk_to_numpy(clusters_vtk)
unique, counts = np.unique(clusters, return_counts = True)
most_common = unique[np.argmax(counts)]

th = vtk.vtkThresholdPoints()
th.SetInputData(pdi)
th.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "cluster");
th.ThresholdBetween(most_common, most_common)
th.Update()

self.GetPolyDataOutput()
pdo.ShallowCopy(th.GetOutput())

"""
PF_selectBiggestCluster.UpdatePipeline()

## Create Road mesh
roadMesh = smp.Delaunay2D(Input=PF_selectBiggestCluster)
roadMesh.ProjectionPlaneMode = 'XY Plane'
roadMesh.Alpha = 5
roadMesh.UpdatePipeline()

smp.RenameSource('Road mesh', roadMesh)

# ---- Finally label the road points and the points that are over the road -----
# Having a mesh of the ground plane enables discriminating the points that
# are over the road from the others by checking if their projection on the
# ground plane belongs to the road or not.
PF_colorPoints = smp.ProgrammableFilter(Input=[roadMesh, PF_rotateFitGround] )

PF_colorPoints.RequestInformationScript = ''
PF_colorPoints.RequestUpdateExtentScript = ''
PF_colorPoints.PythonPath = ''

PF_colorPoints.Script = """
import vtk

input0 = inputs[0]
input1 = inputs[1]

if input0.GetNumberOfPoints() > input1.GetNumberOfPoints():
    inputpc = input0
    inputroad = input1
else:
    inputpc = input1
    inputroad = input0

isOnPlane = vtk.vtkIntArray()
isOnPlane.SetNumberOfComponents(1)
isOnPlane.SetName('pointStatus')


points = inputpc.GetPointData()
selectPoints = vtk.vtkSelectEnclosedPoints()
selectPoints.SetTolerance(1e-12)

selectPoints.SetSurfaceData(inputroad.VTKObject) # Not used but prevents vtk error
selectPoints.SetInputData(inputpc.VTKObject)     # Not used but prevents vtk error

selectPoints.Initialize(inputroad.VTKObject)
selectPoints.Update()

for ii in range(inputpc.GetNumberOfPoints()):
    coords = inputpc.GetPoint(ii)
    x, y, z = coords
    a = selectPoints.IsInsideSurface(x, y, 0)
    status = 0
    if a:
        if (z <= 0.1): # road level
            status = 1
        elif (z <= 2):
            status = 2

    isOnPlane.InsertValue(ii, status)

selectPoints.Complete()

output.DeepCopy(inputpc.VTKObject)
output.GetPointData().AddArray(isOnPlane)

"""

PF_colorPoints.UpdatePipeline()
smp.RenameSource('labeled pointcloud', PF_colorPoints)


# ==============================================================================
# Rendering
# ==============================================================================

# ----- Render with labels coloring
coloredPointsDisplay = smp.Show(PF_colorPoints)
smp.Hide(pointCloud)

pointStatusLUT = smp.GetColorTransferFunction('pointStatus')
pointStatusLUT.InterpretValuesAsCategories = 1
pointStatusLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 1.0, 0.865003, 0.865003, 0.865003, 2.0, 0.705882, 0.0156863, 0.14902]
pointStatusLUT.NanColor = [0.6, 0.6, 0.6]
pointStatusLUT.ScalarRangeInitialized = 1.0
pointStatusLUT.Annotations = ['2', 'obstacle', '0', 'other', '1', 'road']
pointStatusLUT.ActiveAnnotatedValues = ['0', '1', '2']
pointStatusLUT.IndexedColors = [0.6352941176470588, 0.0, 0.0, 0.615686274509804, 1.0, 0.7568627450980392, 0.0, 0.3333333333333333, 1.0]

smp.ColorBy(coloredPointsDisplay, ('POINTS', 'pointStatus'))

smp.Render()

