"""
Select points with a query and save all the frame of the current selection
"""

import os

import paraview.simple as smp
import lidarviewcore.kiwiviewerExporter as kiwiExporter

################################################################################
default_filename = "C:\Demo\selected_frames.csv"
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

def saveCSVSelectionAndFrames(selection, filename, timesteps = default_timesteps):
    """ Save the selection for each frame defined in timesteps to a zip directory """
    # Create a temporary directory to save all frames
    tmp_dir = kiwiExporter.tempfile.mkdtemp()
    basename_without_extension = os.path.splitext(os.path.basename(filename))[0]
    out_dir = os.path.join(tmp_dir, basename_without_extension)
    name_template = os.path.join(out_dir, basename_without_extension + " (Frame %04d).csv")
    os.makedirs(out_dir)

    # Create a new writer to save frames
    csvWriter = smp.servermanager.writers.CSVWriter(Input=selection)
    csvWriter.Precision = 16
    csvWriter.FieldAssociation = "Point Data"

    # Get player controller to be able to search for each frame
    scene = smp.GetAnimationScene()

    #get the lidar source for timestamp
    lidarSource = smp.GetActiveSource()

    for i in timesteps:
        # Load each frame
        timestamp = lidarSource.TimestepValues[i]
        scene.AnimationTime = timestamp # or scene.GoToNext()

        # Name the current frame with its number
        csvWriter.FileName = name_template % i
        csvWriter.UpdatePipeline(timestamp)

    # Clean up writer
    smp.Delete(csvWriter)

    # Zip the directory with all frames and clean up temporary directory
    zip_name = os.path.dirname(filename) + '/' + basename_without_extension + ".zip"
    kiwiExporter.zipDir(out_dir, zip_name)
    kiwiExporter.shutil.rmtree(tmp_dir)

def SaveSelectPointsOnMultipleFrames(filename, timesteps = default_timesteps):
    """ How to use this example """
    renderSelection() # This could be replaced by a manual selection
    selection = extractSelection()
    saveCSVSelectionAndFrames(selection, filename, timesteps)

if __name__ == "__main__":
    SaveSelectPointsOnMultipleFrames(default_filename)
