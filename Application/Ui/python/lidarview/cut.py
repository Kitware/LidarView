import paraview.simple as smp
from vtk.numpy_interface import dataset_adapter as dsa
import numpy as np

from paraview import servermanager

import lidarviewpythonplugin
smp.LoadPlugin(lidarviewpythonplugin.__file__, remote=False, ns=globals())

def init():
    renderview1 = smp.FindViewOrCreate('RenderView1', viewtype='RenderView')
    smp.SetActiveView(renderview1)

    #Clean if there are other views opened in window
    # Note : if the layout was splitted but no view was assigned 
    # a vtk error will be printed when splitting again
    layout = smp.GetLayout()
    views = smp.GetViewsInLayout(layout)
    for v in views:
        if v != renderview1:
            i = layout.GetViewLocation(v)
            smp.Delete(v)
            layout.Collapse(i)

    slam = smp.FindSource('SLAMonline1')

    # Set filters with default values, that will be updated
    clip = smp.Clip(Input=smp.OutputPort(slam, 1))
    smp.RenameSource("SectionClip", clip)
    clip.ClipType = 'Sphere'
    # Center not in 0 to be sure to have planar points in the section for first section
    clip.ClipType.Center = [0., 0., 0.]
    clip.ClipType.Radius = 3.
    clip.UpdatePipeline()

    orthoSectTraj = smp.ExtractSurface(Input=clip)
    smp.RenameSource("ExtractTrajectory", orthoSectTraj)

    orthoSect = smp.OrthogonalSectioning(Points=smp.OutputPort(slam, 3),
                                         Trajectory=orthoSectTraj,
                                         Origin=[0., 0., 0.],
                                         Width=1.)

    smp.RenameSource("OrthogonalSectioning", orthoSect)
    orthoSect.UpdatePipeline()
    # Display section in new renderview
    layout.SplitHorizontal(0, 0.5)
    renderview2 = smp.FindViewOrCreate('RenderView2', viewtype='RenderView')

    renderview2.AxesGrid.Visibility = 1
    renderview2.CenterAxesVisibility = 1
    renderview2.InteractionMode = '2D'
    renderview2.AxesGrid.ShowGrid = 1
    renderview2.AxesGrid.GridColor = [0.7, 0.7, 0.7]
    renderview2.Background = [0.0, 0.0, 0.0]
    renderview2.Background2 = [0.0, 0.0, 0.2]
    renderview2.UseGradientBackground = True
    layout.AssignView(2, renderview2)
    orthoSectViewDisplay = smp.Show(orthoSect, renderview2)
    orthoSectViewDisplay.PointSize = 3.0
    smp.ColorBy(orthoSectViewDisplay, ('POINTS', 'intensity'))
    renderview2.ResetCamera()
    renderview2.Update()

    renderview1 = smp.FindViewOrCreate('RenderView1', viewtype='RenderView')
    smp.SetActiveView(renderview1)
    smp.SetActiveSource(clip)
    smp.Show3DWidgets(proxy=clip.ClipType)
    renderview1.Update()

    # Add property link between clip and orthoSect filter
    pxm = servermanager.ProxyManager()
    propertyLink = servermanager.vtkSMPropertyLink()
    propertyLink.AddLinkedProperty(clip.ClipType.SMProxy, "Center", 1)  # Input
    propertyLink.AddLinkedProperty(orthoSect.SMProxy, "Origin", 2)  # Output
    pxm.RegisterLink("clipSectionLink", propertyLink)

def close():
    renderview1 = smp.FindViewOrCreate('RenderView1', viewtype='RenderView')
    smp.SetActiveView(renderview1)

    orthoSect = smp.FindSource('OrthogonalSectioning')
    if orthoSect:
        smp.Delete(orthoSect)

    clip = smp.FindSource('SectionClip')
    if clip:
        smp.Delete(clip)

    renderView2 = smp.FindView('RenderView2')
    layout = smp.GetLayout()
    views = smp.GetViewsInLayout(layout)
    for v in views:
        if v == renderView2:
            i = layout.GetViewLocation(v)
            smp.Delete(v)
            layout.Collapse(i)