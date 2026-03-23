"""
Delete all render windows views and recreate a default one
"""

import paraview.simple as smp
import lidarview.simple as lvsmp

def ResetWindowViews():
    # Get sources
    reader = smp.GetActiveSource()

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
    renderView = smp.CreateView('LidarGridView')
    layout.AssignView(0, renderView)
    smp.Render(renderView)

    # Show reader
    if reader:
        display = smp.Show(reader, renderView)
        smp.ColorBy(display, ('POINTS', 'intensity'))

    # Reset Camera
    lvsmp.ResetCameraToForwardView()

if __name__ == "__main__":
    ResetWindowViews()
