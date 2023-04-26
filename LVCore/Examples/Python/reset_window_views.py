"""
Delete all render windows views and recreate a default one  
"""

from paraview.simple import *
from lidarview.simple import *
import lidarview.applogic as lv

# Get sources
reader = lv.getReader()
measurementGrid = FindSource('Measurement Grid')

# Get views
views = GetViews()
layout = GetLayout()

# Deletes all views
for view in views:
    idx = layout.GetViewLocation(view)
    Delete(view)
    del view
    layout.Collapse(idx)

# Create and assign a new one
renderView = CreateView('RenderView')
layout.AssignView(0, renderView)

# Show measurement grid and reader
if reader:
    display = Show(reader[0], renderView)
    ColorBy(display, ('POINTS', 'intensity'))
Show(measurementGrid, renderView)

# Reset Camera
ResetCameraToForwardView()
