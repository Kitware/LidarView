from paraview.util.vtkAlgorithm import *

from vtk.numpy_interface import algorithms as algs
from vtk.numpy_interface import dataset_adapter as dsa

from vtkmodules.vtkCommonDataModel import vtkPolyData

import vtk
import numpy as np
from scipy.spatial.transform import Rotation as R


@smproxy.filter(name="SectionalView", label="Sectional View")
@smproperty.input(name="Input", port_index=0)
class SectionalViewFilter(VTKPythonAlgorithmBase):
    """ Create a section view of the input function along the given plane by
    selecting the points within a given distance of the plane and rotating the
    output so that the plane is aligned with the XY plane
    """
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self,
                nInputPorts=1,
                nOutputPorts=1,
                inputType='vtkPolyData',
                outputType='vtkPolyData')
        self.planeOrigin = np.zeros(3)
        self.planeNormal = np.array([0, 0, 1])
        self.matchPlaneUpWithDataZ = True
        self.width = 0.05


    def RequestData(self, request, inInfo, outInfo):
        input0 = dsa.WrapDataObject(vtkPolyData.GetData(inInfo[0]))
        output = dsa.WrapDataObject(vtkPolyData.GetData(outInfo))
        output.DeepCopy(input0.VTKObject)
        pts = input0.Points
        dist = np.dot((pts - self.planeOrigin),
                       self.planeNormal)
        ptsToKeep = np.abs(dist) <= self.width
        pts = pts[ptsToKeep, :]
        dist = dist[ptsToKeep]

        # Compute rotation quateronion to project points onto XY plane
        # Using help from https://stackoverflow.com/questions/10236557/getting-quaternion-to-rotate-between-two-vectors
        zaxis = np.array([0, 0, 1])
        if np.all(self.planeNormal == zaxis):
            rotVector = np.zeros(3)

        elif np.all(self.planeNormal == -zaxis):
            rotVector = np.array([1, 0, 0]) * np.pi

        else:
            rotAxis = np.cross(self.planeNormal, zaxis)
            rotAxis = rotAxis / np.linalg.norm(rotAxis)
            rotAngle = np.arccos(self.planeNormal[2])
            rotVector = rotAxis * rotAngle

        rotation = R.from_rotvec(rotVector)
        offset = rotation.apply(self.planeOrigin)
        points = rotation.apply(pts) - offset

        if self.matchPlaneUpWithDataZ:
            projectedZ = rotation.as_matrix()[:, 2]
            projectedZ[2] = 0.
            projectedZ /= np.linalg.norm(projectedZ)
            rotAngle = np.arctan2(projectedZ[0], projectedZ[1])
            rotVector = zaxis * rotAngle
            rotation2D = R.from_rotvec(rotVector)
            points = rotation2D.apply(points)

        output.Points = points

        for key in input0.PointData.keys():
            output.PointData.append(input0.PointData[key][ptsToKeep], key)

        # Set polyvertex cell
        ptIds = vtk.vtkIdList()
        ptIds.SetNumberOfIds(len(dist))
        for a in range(len(dist)):
            ptIds.SetId(a, a)

        output.Allocate(1)
        output.InsertNextCell(vtk.VTK_POLY_VERTEX, ptIds)

        return 1

    @smproperty.doublevector(name="Origin", default_values=[0, 0, 0])
    def SetOrigin(self, x, y, z):
        """ Set the origin of the section plane """
        self.planeOrigin = np.array([x, y, z])
        self.Modified()

    @smproperty.doublevector(name="Normal", default_values=[0, 0, 1])
    def SetNormal(self, x, y, z):
        """ Set the normal vector of the section plane """
        self.planeNormal = np.array([x, y, z]) / np.linalg.norm([x, y, z])
        self.Modified()

    @smproperty.doublevector(name="Width", default_values=0.05)
    @smdomain.doublerange()
    def SetWidth(self, w):
        """ Half-Width of the section, the output contains all the points that
        have a distance <= Width with the section plane"""
        if w <= 0:
            print("Please set a positive value for the section width.")
        else:
            self.width = w
            self.Modified()

    @smproperty.xml("""
        <IntVectorProperty name="MatchPlaneUpWithDataZ"
            number_of_elements="1"
            default_values="1"
            command="SetMatchPlaneUpWithDataZ">
         <BooleanDomain name='bool'/>
         <Documentation>
            Do rotate the output plane in order to have the up direction
            corresponding to the Z axis of the input data?
         </Documentation>
      </IntVectorProperty>
      """)
    def SetMatchPlaneUpWithDataZ(self, val):
        self.matchPlaneUpWithDataZ = val
        self.Modified()
