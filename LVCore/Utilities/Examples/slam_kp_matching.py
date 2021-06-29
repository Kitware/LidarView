"""
Display the SLAM current keypoints, and color them by matching status.
This is useful to debug why the registration of a scan may fail.
Example :
- open a lidar pcap
- apply SLAM Online filter on the Data/Frame entry
- select the SLAM entry in the pipeline browser
- run this script (tip: save it as macro to easily use it)
"""

from paraview.simple import *

# get active source.
SlamOnline = GetActiveSource()
SlamOnline.Advancedreturnmode = 1
SlamOnline.Outputcurrentkeypoints = 1
SlamOnline.Keypointsmapsupdatestep = 1

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# update the view to ensure updated data information
renderView1.Update()

# hide data in view
Hide(SlamOnline, renderView1)

### EDGES KP

# get display properties
SlamOnlineEdgesKpDisplay = GetDisplayProperties(OutputPort(SlamOnline, 5), view=renderView1)
# set scalar coloring
ColorBy(SlamOnlineEdgesKpDisplay, ('POINTS', 'Localization: edge matches'))

# get color transfer function/color map for 'Localizationedgematches'
localizationedgematchesLUT = GetColorTransferFunction('Localizationedgematches')
localizationedgematchesPWF = GetOpacityTransferFunction('Localizationedgematches')

# Properties modified on localizationedgematchesLUT
localizationedgematchesLUT.InterpretValuesAsCategories = 1
localizationedgematchesLUT.AnnotationsInitialized = 1
localizationedgematchesLUT.Annotations = ['0', 'SUCCESS',
                                          '1', 'BAD_MODEL_PARAMETRIZATION',
                                          '2', 'NOT_ENOUGH_NEIGHBORS',
                                          '3', 'NEIGHBORS_TOO_FAR',
                                          '4', 'BAD_PCA_STRUCTURE',
                                          '5', 'INVALID_NUMERICAL',
                                          '6', 'MSE_TOO_LARGE',
                                          '7', 'UNKNOWN']
localizationedgematchesLUT.IndexedColors = [0.0, 1.0, 0.0,
                                            0.5, 0.2, 0.0,
                                            1.0, 0.0, 0.0,
                                            1.0, 1.0, 0.0,
                                            1.0, 0.0, 1.0,
                                            0.0, 0.3, 1.0,
                                            0.0, 1.0, 1.0,
                                            1.0, 1.0, 1.0]
localizationedgematchesLUT.IndexedOpacities = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]

# show color bar/color legend
SlamOnlineEdgesKpDisplay.SetScalarBarVisibility(renderView1, False)

### PLANES KP

# get display properties
SlamOnlinePlanesKpDisplay = GetDisplayProperties(OutputPort(SlamOnline, 6), view=renderView1)
# set scalar coloring
ColorBy(SlamOnlinePlanesKpDisplay, ('POINTS', 'Localization: plane matches'))

# get color transfer function/color map for 'Localizationplanematches'
localizationplanematchesLUT = GetColorTransferFunction('Localizationplanematches')
localizationplanematchesPWF = GetOpacityTransferFunction('Localizationplanematches')

# Properties modified on localizationplanematchesLUT
localizationplanematchesLUT.InterpretValuesAsCategories = 1
localizationplanematchesLUT.AnnotationsInitialized = 1
localizationplanematchesLUT.Annotations = localizationedgematchesLUT.Annotations
localizationplanematchesLUT.IndexedColors = localizationedgematchesLUT.IndexedColors
localizationplanematchesLUT.IndexedOpacities = localizationedgematchesLUT.IndexedOpacities

# show color bar/color legend
SlamOnlinePlanesKpDisplay.SetScalarBarVisibility(renderView1, True)

### EDGES MAP

# show data in view
SlamOnlineEdgesMapDisplay = Show(OutputPort(SlamOnline, 2), renderView1)

# turn off scalar coloring
ColorBy(SlamOnlineEdgesMapDisplay, None)
SlamOnlineEdgesMapDisplay.PointSize = 2.0
SlamOnlineEdgesMapDisplay.DiffuseColor = [1., 1., 1.]

### PLANES MAP

# show data in view
SlamOnlinePlanesMapDisplay = Show(OutputPort(SlamOnline, 3), renderView1)

# turn off scalar coloring
ColorBy(SlamOnlinePlanesMapDisplay, None)
SlamOnlinePlanesMapDisplay.PointSize = 1.0
SlamOnlinePlanesMapDisplay.DiffuseColor = [1., 1., 1.]

# set active source
SetActiveSource(SlamOnline)