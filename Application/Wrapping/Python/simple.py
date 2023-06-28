#==============================================================================
#
#  Program: LidarView
#  Module:  simple.py
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

import paraview.simple as smp

# Import all lidarviewcore common functions
from lidarviewcore.simple import *

INTERPRETER_MAP = {
    "Velodyne": "Velodyne Meta Interpreter",
    "Hesai-pandarXT32": "Hesai General Packet Interpreter",
    "Hesai-pandar128": "Hesai Packet Interpreter"
}

# -----------------------------------------------------------------------------
# This function should disapear when we will refact interpreters and
# set simpler names.
def GetInterpreterName(interpreter):
    """Utility method to help selecting the right interpreter.

    Options are: `Velodyne`, `Hesai-pandarXT32` and `Hesai-pandar128`
    """
    interpreterName = INTERPRETER_MAP.get(interpreter, "")
    if not interpreterName:
        raise ValueError("A valid interpreter must be specified.")
    return interpreterName
