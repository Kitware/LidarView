#==============================================================================
#
#  Program: LidarView
#  Module:  __init__.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================

""" lidarviewplugins
This python module contains paraview "programmable filter"-like plugins:
Subclasses of `VTKPythonAlgorithmBase`.
In order to use them in a lidarview app, please use LoadLidarViewPythonPlugins()
function from lidarview simple.

In LidarView python shell or in python script:
```
# Not needed in LidarView app
from lidarviewcore.simple import *

LoadLidarViewPythonPlugins() # Load all available plugins

LoadLidarViewPythonPlugins("SectionalView") # Only load SectionalView
```

then you can use the capabilities of the plugin as any other paraview plugin:
- in a python shell with `smp.YourFilterName(...)`
- they will appear in the menus in the lidarview UI
"""
from lidarviewplugins.SectionalView import *
