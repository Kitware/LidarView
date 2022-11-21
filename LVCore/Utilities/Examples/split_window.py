"""
Split the view in multiple rend windows and change display Properties for each  
"""

from paraview.simple import *
import lidarview.applogic as lv

# Get sources
reader = lv.getReader()
measurementGrid = FindSource('Measurement Grid')

# Get views
renderView1 = GetActiveView()
layout1 = GetLayout()

# Split cells
layout1.SplitHorizontal(0, 0.5)
layout1.SplitVertical(1, 0.5)
layout1.SplitVertical(2, 0.5)

# Create a new 'Render View'
renderView2 = CreateView('RenderView')
renderView2.Background = [0.05, 0.43, 0.32]
renderView3 = CreateView('RenderView')
renderView3.Background = [0.43, 0.05, 0.34]
renderView4 = CreateView('RenderView')
renderView4.Background = [0.32, 0.34, 0.05]

# Place view in the layout
layout1.AssignView(4, renderView2)
layout1.AssignView(5, renderView3)
layout1.AssignView(6, renderView4)


# Show data in the 2 renderview

if reader:
    display1 = Show(reader[0], renderView1)
    ColorBy(display1, ('POINTS', 'azimuth'))
    display2 = Show(reader[0], renderView2)
    ColorBy(display2, ('POINTS', 'intensity'))
    display3 = Show(reader[0], renderView3)
    ColorBy(display3, ('POINTS', 'azimuth'))
    display4 = Show(reader[0], renderView4)
    ColorBy(display4, ('POINTS', 'intensity'))

# Show measurement grid
Show(measurementGrid, renderView1)
Show(measurementGrid, renderView2)
Show(measurementGrid, renderView3)
Show(measurementGrid, renderView4)


# Add camera links
AddCameraLink(renderView1, renderView2, "link_1")
AddCameraLink(renderView3, renderView4, "link_2")

# Change camera position
renderView1.CameraPosition = [-15, 150, 30]
renderView1.CameraViewUp = [0, 0, 1]
renderView3.CameraPosition = [-15, -150, 30]
renderView3.CameraViewUp = [0, 0, 1]