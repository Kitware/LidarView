"""
Split the view in multiple rend windows and change display Properties for each
"""

import paraview.simple as smp

def SplitWindow():
    # Get sources
    reader = smp.GetActiveSource()

    # Get views
    renderView1 = smp.GetActiveView()
    layout1 = smp.GetLayout()

    # Split cells
    layout1.SplitHorizontal(0, 0.5)
    layout1.SplitVertical(1, 0.5)
    layout1.SplitVertical(2, 0.5)

    # Create a new 'Render View'
    renderView2 = smp.CreateView('LidarGridView')
    renderView2.Background = [0.05, 0.43, 0.32]
    renderView3 = smp.CreateView('LidarGridView')
    renderView3.Background = [0.43, 0.05, 0.34]
    renderView4 = smp.CreateView('LidarGridView')
    renderView4.Background = [0.32, 0.34, 0.05]

    # Place view in the layout
    layout1.AssignView(4, renderView2)
    layout1.AssignView(5, renderView3)
    layout1.AssignView(6, renderView4)


    # Show data in the 2 renderview

    if reader:
        display1 = smp.Show(reader[0], renderView1)
        smp.ColorBy(display1, ('POINTS', 'azimuth'))
        display2 = smp.Show(reader[0], renderView2)
        smp.ColorBy(display2, ('POINTS', 'intensity'))
        display3 = smp.Show(reader[0], renderView3)
        smp.ColorBy(display3, ('POINTS', 'azimuth'))
        display4 = smp.Show(reader[0], renderView4)
        smp.ColorBy(display4, ('POINTS', 'intensity'))

    # Add camera links
    smp.AddCameraLink(renderView1, renderView2, "link_1")
    smp.AddCameraLink(renderView3, renderView4, "link_2")

    # Change camera position
    renderView1.CameraPosition = [-15, 150, 30]
    renderView1.CameraViewUp = [0, 0, 1]
    renderView3.CameraPosition = [-15, -150, 30]
    renderView3.CameraViewUp = [0, 0, 1]

if __name__ == "__main__":
    SplitWindow()
