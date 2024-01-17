"""
Select points with a query and save all the frame of the current selection

You can download CarLoop_VLP16.pcap here: https://drive.google.com/drive/folders/1yrNUelUsjKcXdC8FH8DpXeOPTyiB_pLS
"""

import lidarview.planefit as planefit
import lidarview.simple as lvsmp
import paraview.simple as smp

################################################################################
carloop_pcap = "/home/user/data/CarLoop_VLP16.pcap"
lidar_model = "VLP-16"
calibration_file = "/path/to/lidarview/build/.../install/share/VLP-16.xml"
default_filename = "/home/user/data/test.csv"
interpreter = "Velodyne"
################################################################################

def extractFromSelectMultipleQueries(queries = []):
    """ Workaround to do a selection based on multiple queries """
    selection = None
    for query in queries:
        smp.SelectPoints(query)
        smp.Render()
        tmp = smp.ExtractSelection()
        # Delete intermediate selection
        if selection:
            smp.Delete(selection)
        selection = tmp
    return selection

def planeFitSelection(source):
    """ Do a plane fit on the source and return a plane fitter """
    # Select all point on extracted source
    smp.SelectPoints("", source)
    smp.Render()

    # Call planeFit implementation (see Wrapping/Python/planefit.py)
    planefit.fitPlane()

    # Return plane fit source
    return smp.FindSource("PlaneFitter")

def executeProgrammableFilter(plane_fitter):
    """ Create a programmable filter to perform computation on data """
    prog_filter = smp.ProgrammableFilter(Input=[plane_fitter])

    # Docs to use programmable filters https://docs.paraview.org/en/latest/ReferenceManual/pythonProgrammableFilter.html#
    # Docs to use numpy in paraview https://docs.paraview.org/en/latest/ReferenceManual/vtkNumPyIntegration.html
    prog_filter.Script = """
from paraview.vtk.numpy_interface import dataset_adapter as dsa
from paraview.vtk.numpy_interface import algorithms as algs

import numpy as np

data = inputs[0]

npts = data.RowData["npts"]
normaly = data.RowData["normaly"]
print(type(npts))                       # <class 'paraview.vtk.numpy_interface.dataset_adapter.VTKArray'>
print(isinstance(npts, numpy.ndarray))  # True
approxrollangle = numpy.arcsin(normaly) * 180/3.1415    # vtk datasets are tied to numpy array

# Other treatment here

output.RowData.append(npts, "npts")       # Append data in output
output.RowData.append(approxrollangle, "roll_angle") # Append data in output
    """

    prog_filter.UpdatePipeline()
    return prog_filter

def saveProgrammableFilterOutput(filename, prog_filter):
    """ Save programmabl filter ouput to a csv """
    csvWriter = smp.servermanager.writers.CSVWriter(Input=prog_filter, FileName=filename)
    csvWriter.Precision = 16
    csvWriter.UpdatePipeline()
    smp.Delete(csvWriter)


def PlaneFitAndProcess(pcapfile, model, outputFilename, calib):
    """ How to use this example """
    # Open pcap
    lvsmp.OpenPCAP(pcapfile, model, interpreter, CalibrationFileName=calib)

    # Select region on hardcoded requirement (in this case in front of lidar)
    selection = extractFromSelectMultipleQueries(["azimuth > 34500", "vertical_angle < -10"])

    plane_fitter = planeFitSelection(selection)
    prog_filter = executeProgrammableFilter(plane_fitter)
    saveProgrammableFilterOutput(outputFilename, prog_filter)

if __name__ == "__main__":
    PlaneFitAndProcess(carloop_pcap, lidar_model, default_filename, calibration_file)
