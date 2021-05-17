import paraview.simple as smp
from vtk.numpy_interface import dataset_adapter as dsa
import numpy as np

from paraview import servermanager

import lidarviewpythonplugin
smp.LoadPlugin(lidarviewpythonplugin.__file__, remote=False, ns=globals())

def init():
    slamView = smp.GetActiveView()
    if slamView is None:
        slamView = smp.FindViewOrCreate('SlamView', viewtype='RenderView')
        smp.SetActiveView(slamView)
    smp.RenameView("SlamView", slamView)
    #Clean if there are other views opened in window
    # Note : if the layout was splitted but no view was assigned 
    # a vtk error will be printed when splitting again
    layout = smp.GetLayout()
    views = smp.GetViewsInLayout(layout)
    for v in views:
        if v != slamView:
            i = layout.GetViewLocation(v)
            smp.Delete(v)
            layout.Collapse(i)

    slam = smp.FindSource('SLAMonline1')
    # Display Planar map on SlamView
    PlanarMapDisplay = smp.Show(smp.OutputPort(slam, 3), slamView)
    PlanarMapDisplay.Representation = 'Surface'
    smp.ColorBy(PlanarMapDisplay, ('POINTS', 'intensity'))
    PlanarMapDisplay.RescaleTransferFunctionToDataRange(True, False)
    slamView.Background = [0.0, 0.0, 0.0]
    slamView.Background2 = [0.0, 0.0, 0.2]
    slamView.UseGradientBackground = True
    slamView.ResetCamera()

    TrajectoryDisplay = smp.Show(smp.OutputPort(slam, 1), slamView)
    TrajectoryDisplay.Representation = 'Points'
    TrajectoryDisplay.PointSize = 5.0
    TrajectoryDisplay.AmbientColor = [1.0, 1.0, 1.0]

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
    sectionView = smp.FindViewOrCreate('SectionView', viewtype='RenderView')
    smp.RenameView("SectionView", sectionView)
    sectionView.AxesGrid.Visibility = 1
    sectionView.CenterAxesVisibility = 1
    sectionView.InteractionMode = '2D'
    sectionView.AxesGrid.ShowGrid = 1
    sectionView.AxesGrid.GridColor = [0.7, 0.7, 0.7]
    sectionView.Background = [0.0, 0.0, 0.0]
    sectionView.Background2 = [0.0, 0.0, 0.2]
    sectionView.UseGradientBackground = True
    orthoSectViewDisplay = smp.Show(orthoSect, sectionView)
    orthoSectViewDisplay.PointSize = 3.0
    smp.ColorBy(orthoSectViewDisplay, ('POINTS', 'intensity'))
    sectionView.ResetCamera()
    sectionView.Update()
    
    smp.SetActiveView(slamView)
    smp.SetActiveSource(clip)
    smp.Show3DWidgets(proxy=clip.ClipType)
    slamView.Update()
    
    # Add property link between clip and orthoSect filter
    pxm = servermanager.ProxyManager()
    propertyLink = servermanager.vtkSMPropertyLink()
    propertyLink.AddLinkedProperty(clip.ClipType.SMProxy, "Center", 1)  # Input
    propertyLink.AddLinkedProperty(orthoSect.SMProxy, "Origin", 2)  # Output
    pxm.RegisterLink("clipSectionLink", propertyLink)

def close():
    slamView = smp.FindView('SlamView')
    if slamView is None:
        slamView = smp.FindViewOrCreate('RenderView1', viewtype='RenderView')
    smp.SetActiveView(slamView)

    orthoSect = smp.FindSource('OrthogonalSectioning')
    if orthoSect:
        smp.Delete(orthoSect)

    clip = smp.FindSource('SectionClip')
    if clip:
        smp.Delete(clip)

    sectionView = smp.FindView('SectionView')
    layout = smp.GetLayout()
    views = smp.GetViewsInLayout(layout)
    for v in views:
        if v == sectionView:
            i = layout.GetViewLocation(v)
            smp.Delete(v)
            layout.Collapse(i)