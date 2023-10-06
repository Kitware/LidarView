# Copyright 2013 Velodyne Acoustics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys, os, csv, time, datetime, math, bisect

import paraview
import paraview.simple as smp
from paraview import vtk

import PythonQt
from PythonQt import QtCore, QtGui

import lidarview.simple as lvsmp

# import the vtk wrapping of the Lidar Plugin
# this enable to get the specific vtkObject behind a proxy via GetClientSideObject()
# without this plugin, GetClientSideObject(), would return the first mother class known by paraview
import lidarview.modules.lvCommonCore
import lidarview.modules.lvIOCore
import lidarview.modules.lvIOGeolocation
import lidarview.modules.lvIONetwork
import lidarview.modules.lvIOLidar

class AppLogic(object):

    def __init__(self):
        pass

def createGrid():
    app.grid = smp.GridSource(guiName='Measurement Grid')

    # Reset to default if not persistent asked
    if (getPVSettings().value('LidarPlugin/grid/gridPropertiesPersist') != "true"):
      # Default Settings # WIP NEED TO INIT OTHER PROPS ?
      app.grid.GridNbTicks = 10
    else:
        # Restore grid properties
        lineWidth = getPVSettings().value('LidarPlugin/grid/LineWidth')
        if lineWidth :
            app.grid.LineWidth = int(lineWidth)

        gridNbTicks = getPVSettings().value('LidarPlugin/grid/GridNbTicks')
        if gridNbTicks :
            app.grid.GridNbTicks = int(gridNbTicks)

        if getPVSettings().value('LidarPlugin/grid/Normal'):
            normal_x = getPVSettings().value('LidarPlugin/grid/Normal')[0]
            normal_y = getPVSettings().value('LidarPlugin/grid/Normal')[1]
            normal_z = getPVSettings().value('LidarPlugin/grid/Normal')[2]
            app.grid.Normal = [float(normal_x), float(normal_y), float(normal_z)]

        if getPVSettings().value('LidarPlugin/grid/Origin'):
            origin_x = getPVSettings().value('LidarPlugin/grid/Origin')[0]
            origin_y = getPVSettings().value('LidarPlugin/grid/Origin')[1]
            origin_z = getPVSettings().value('LidarPlugin/grid/Origin')[2]
            app.grid.Origin = [float(origin_x), float(origin_y), float(origin_z)]

        scale = getPVSettings().value(getPVSettings().value('LidarPlugin/grid/Scale'))
        if scale :
            app.grid.Scale = float(scale)

        if getPVSettings().value('LidarPlugin/grid/gridColor') :
            r = getPVSettings().value('LidarPlugin/grid/gridColor')[0]
            g = getPVSettings().value('LidarPlugin/grid/gridColor')[1]
            b = getPVSettings().value('LidarPlugin/grid/gridColor')[2]
            app.grid.Color = [float(r), float(g), float(b)]

    rep = smp.Show(app.grid)
    rep.LineWidth = app.grid.LineWidth
    rep.DiffuseColor = app.grid.Color
    rep.Pickable = 0
    rep.Visibility = 0
    smp.SetActiveSource(None)

    app.grid.UpdatePipeline()
    smp.Show(app.grid)
    return app.grid

# Main function, Used by lqLidarViewManager
def start():

    global app
    app = AppLogic()

    smp._DisableFirstRenderCameraReset()
    smp.GetActiveView().LODThreshold = 1e100

    lvsmp.ResetCameraToForwardView()

    setupActions()

    # Create Grid #WIP not perfect requires loaded plugin
    createGrid()


def findQObjectByName(widgets, name):
    for w in widgets:
        if w.objectName == name:
            return w


def getMainWindow():
    return findQObjectByName(QtGui.QApplication.topLevelWidgets(), 'LidarViewMainWindow')


def getPVApplicationCore():
    return PythonQt.paraview.pqPVApplicationCore.instance()


def getPVSettings():
    return getPVApplicationCore().settings()

# Setup Actions
def setupActions():

    mW = getMainWindow()
    actions = mW.findChildren('QAction')

    app.actions = {}

    for a in actions:
        app.actions[a.objectName] = a
