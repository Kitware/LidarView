# Copyright 2013 Velodyne Acoustics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Imports
import paraview.simple as smp
from paraview import vtk, servermanager

# Use Named filters
planefitter_name = 'PlaneFitter'       # PlaneFitter Instance
selection_copy_name = 'SelectedPointsCopy' # Trivial copy of the selected points

# Clean planefitter's state
def cleanState():
  # Remove PlaneFitter
  planefitter = smp.FindSource(planefitter_name)
  if planefitter is not None:
    smp.Delete(planefitter)
    del planefitter

  # Remove Selection copy TrivialProducer
  trivialproducer = smp.FindSource(selection_copy_name)
  if trivialproducer is not None:
    smp.Delete(trivialproducer)
    del trivialproducer

# Find or create a 'SpreadSheet View' to display plane fitting statistics
def showStats(planefitter, actionSpreadsheet=None):

  # Check for SpreadSheetReaction
  if actionSpreadsheet is None:
    print("Unable to display stats : SpreadSheet action is not defined")
    return

  # Check if main spreadsheet view exist or can be created
  spreadSheetView1 = smp.FindView('main spreadsheet view')
  if spreadSheetView1 is None:
    # try to trigger actionSpreadsheet to display main spreadsheet view
    actionSpreadsheet.trigger()
    spreadSheetView1 = smp.FindView('main spreadsheet view')
    if spreadSheetView1 is None:
      print("Unable to get main spreadsheet view")
      return

  # Display stats in main spreadsheet view
  spreadSheetView1.ColumnToSort = ''
  spreadSheetView1.BlockSize = 1024
  spreadSheetView1.FieldAssociation = 'Row Data'
  smp.Show(planefitter, spreadSheetView1)

# Main Method
def fitPlane(actionSpreadsheet=None):
  # Get Selected Source
  src = smp.GetActiveSource()
  if src is None:
    print("A source need to be selected before running plane fitting")
    return

  # Clean plane fitting state before processing a new one
  cleanState()

  # Check Selection
  selection = src.GetSelectionInput(src.Port)
  if selection is None:
    print("A selection has to be defined to run plane fitting")
    return

  # Extract Selection - vtkMultiBlockDataSet(vtkUnstructuredGrid)
  extractor = smp.ExtractSelection()
  extractor.Selection = selection
  extractor.Input = src

  # Extract first Block - vtkUnstructuredGrid
  merger = smp.MergeBlocks(Input=extractor)

  # Convert to polydata - vtkPolyData
  extractsurf = smp.ExtractSurface(Input=merger)
  extractsurf.UpdatePipeline()

  # Create a Trivial Producer HardCopy of selected points
  trivialproducer = smp.PVTrivialProducer()
  smp.RenameSource(selection_copy_name, trivialproducer)
  trivialproducer.GetClientSideObject().SetOutput(extractsurf.GetClientSideObject().GetOutput())
  
  # PlaneFitter - Using a fixed input
  planefitter = smp.PlaneFitter(Input=trivialproducer)
  smp.RenameSource(planefitter_name, planefitter)

  # Determine Laser ID Array Name
  try:
    # if laser_id is the name of the array in Legacy and Special Velarray mode
    # LCN is the name of the array in APF mode
    for arr in extractor.PointData:
      if   arr.Name == "laser_id":
        planefitter.laserIDArray = "laser_id"
      elif arr.Name == "LCN":
        planefitter.laserIDArray = "LCN"
    planefitter.UpdatePipeline()

    # Display results on the main spreadsheet view
    showStats(planefitter, actionSpreadsheet)

    # Show Model
    smp.Show(servermanager.OutputPort(planefitter, outputPort=1))
  except :
    print("PlaneFit error: Unable to select appropriate laserIDArray")
  finally:
    smp.Delete(extractsurf)
    smp.Delete(merger)
    smp.Delete(extractor)
    smp.SetActiveSource(src) # Restore Active Source

