from paraview.util.vtkAlgorithm import *

from vtk.numpy_interface import algorithms as algs
from vtk.numpy_interface import dataset_adapter as dsa

from vtkmodules.vtkCommonDataModel import vtkPolyData

import vtk
import numpy as np

epsilon = 1e-5

def ComputeRotation(rotAngle, rotAxis):
    cosa = np.cos(rotAngle)
    sina = np.sin(rotAngle)
    ux = np.array([[0., -rotAxis[2], rotAxis[1]], [rotAxis[2], 0., -rotAxis[0]], [-rotAxis[1], rotAxis[0], 0.]])
    return cosa * np.identity(3) + sina * ux + (1 - cosa) * np.outer(rotAxis, rotAxis)

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
        self.planeNormal = np.array([1., 0., 0.])
        self.YDirection = np.array([0., 0., 1.])
        self.maxDistance = 0.05


    def RequestData(self, request, inInfo, outInfo):
        input0 = dsa.WrapDataObject(vtkPolyData.GetData(inInfo[0]))
        output = dsa.WrapDataObject(vtkPolyData.GetData(outInfo))
        output.DeepCopy(input0.VTKObject)
        pts = input0.Points
        dist = np.dot((pts - self.planeOrigin),
                       self.planeNormal)
        ptsToKeep = np.abs(dist) <= self.maxDistance
        pts = pts[ptsToKeep, :]
        dist = dist[ptsToKeep]

        # Compute rotation to project points onto XY plane
        zaxis = np.array([0., 0., 1.])
        R = np.identity(3)
        if np.dot(self.planeNormal, zaxis) < -(1. - epsilon):
            R[0][0] = -1.
            R[1][1] = -1.

        elif np.dot(self.planeNormal, zaxis) < (1. - epsilon):
            rotAxis = np.cross(self.planeNormal, zaxis)
            rotAxis = rotAxis / np.linalg.norm(rotAxis)
            rotAngle = np.arccos(self.planeNormal[2])
            R = ComputeRotation(rotAngle, rotAxis)

        offset = np.dot(R, self.planeOrigin)
        points = np.dot(pts, R.transpose()) - offset
        
        projected3dYAxis = np.dot(R, self.YDirection)
        projected3dYAxis[2] = 0.
        if np.linalg.norm(projected3dYAxis) < epsilon:
            print("Can not align 2D Y with 3D axis parameter")
            return 0
        projected3dYAxis /= np.linalg.norm(projected3dYAxis)
        rotAngle = np.arctan2(projected3dYAxis[0], projected3dYAxis[1])
        R = ComputeRotation(rotAngle, zaxis)
        points = np.dot(points, R.transpose())

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

    @smproperty.doublevector(name="Origin", default_values=[0., 0., 0.])
    def SetOrigin(self, x, y, z):
        """ Set the origin of the section plane """
        self.planeOrigin = np.array([x, y, z])
        self.Modified()

    @smproperty.doublevector(name="Normal", default_values=[1., 0., 0.])
    def SetNormal(self, x, y, z):
        """ Set the normal vector of the section plane """
        self.planeNormal = np.array([x, y, z]) / np.linalg.norm([x, y, z])
        self.Modified()

    @smproperty.doublevector(name="MaxDistance", default_values=0.05)
    @smdomain.doublerange()
    def SetMaxDistance(self, d):
        """ Half-width of the section, the output contains all the points that
        have a distance <= maxDistance with the section plane"""
        if d <= 0:
            print("Please set a positive value for MaxDistance",
                  "(distance threshold for points to be displayed.)")
        else:
            self.maxDistance = d
            self.Modified()

    @smproperty.doublevector(name="YDirection", default_values=[0., 0., 1.])
    def SetYDirection(self, nx, ny, nz):
        """ Set the Y direction of the section"""
        self.YDirection = np.array([nx, ny, nz]) / np.linalg.norm([nx, ny, nz])
        self.Modified()