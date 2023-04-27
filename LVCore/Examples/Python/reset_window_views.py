"""
Delete all render windows views and recreate a default one
"""

import paraview.simple as smp
from lidarview.simple import lvsmp
import lidarview.applogic as app

def ResetWindowViews():
    # Get sources
    reader = app.getReader()
    measurementGrid = smp.FindSource('Measurement Grid')

    # Get views
    views = smp.GetViews()
    layout = smp.GetLayout()

    # Deletes all views
    for view in views:
        idx = layout.GetViewLocation(view)
        smp.Delete(view)
        del view
        layout.Collapse(idx)

    # Create and assign a new one
    renderView = smp.CreateView('RenderView')
    layout.AssignView(0, renderView)

    # Show measurement grid and reader
    if reader:
        display = smp.Show(reader[0], renderView)
        smp.ColorBy(display, ('POINTS', 'intensity'))
    smp.Show(measurementGrid, renderView)

    # Reset Camera
    lvsmp.ResetCameraToForwardView()

if __name__ == "__main__":
    ResetWindowViews()
