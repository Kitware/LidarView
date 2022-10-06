"""
Select points with a query and save all the frame of the current selection
"""

import os

import lidarview.applogic as lv
import paraview.simple as smp
import lidarviewcore.kiwiviewerExporter as kiwiExporter

################################################################################
default_filename = "/home/user/data/selected_frames.csv"
default_timesteps = range(50, 100)
################################################################################

def renderSelection(query = "intensity > 20"):
    """ Selects points and render the selection using a query"""
    smp.SelectPoints(query)
    smp.Render()

def extractSelection():
    """ Get the trailing frame with selection then
    create and return the new filter 'ExtractSelection'"""
    trailingFrame = smp.GetActiveSource()
    extractSelection = smp.ExtractSelection(Input=trailingFrame)
    return extractSelection

def saveCSVSelectionAndFrames(selection, filename = default_filename, timesteps = default_timesteps):
    """ Save the selection for each frame defined in timesteps to a zip directory """
    # Create a temporary directory to save all frames
    tmp_dir = kiwiExporter.tempfile.mkdtemp()
    basename_without_extension = os.path.splitext(os.path.basename(filename))[0]
    out_dir = os.path.join(tmp_dir, basename_without_extension)
    name_template = os.path.join(out_dir, basename_without_extension + " (Frame %04d).csv")
    os.makedirs(out_dir)

    # Create a new writer to save frames
    writer = smp.CreateWriter('tmp.csv', selection)
    writer.FieldAssociation = "Point Data"
    writer.Precision = 16

    # Get player controller to be able to search for each frame
    controller = lv.getPlayerController()

    for i in timesteps:
        # Load each frame
        controller.onSeekFrame(i)
        # Name the current frame with its number
        writer.FileName = name_template % i
        writer.UpdatePipeline()
        # Format csv file
        lv.rotateCSVFile(writer.FileName)

    # Clean up writer
    smp.Delete(writer)

    # Zip the directory with all frames and clean up temporary directory
    zip_name = os.path.dirname(filename) + '/' + basename_without_extension + ".zip"
    kiwiExporter.zipDir(out_dir, zip_name)
    kiwiExporter.shutil.rmtree(tmp_dir)

def saveSelectPointsOnMultipleFramesExample():
    """ How to use this example """
    renderSelection() # This could be replaced by a manual selection
    selection = extractSelection()
    saveCSVSelectionAndFrames(selection)

# Uncomment below to execute the script directly when loaded
# saveSelectPointsOnMultipleFramesExample()