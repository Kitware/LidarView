"""
This script is an example of how to easily write a PythonAnimationCue with
the temporal_animation_cue_helpers lib to follow a camera path along time in a
static "frozen" dataset.

You can run this script with the command:
`./install/bin/LidarView --script=${LV_SOURCES}/LVCore/Utilities/Animation/examples/example_static_temporal_animation.py`
"""
import paraview.simple as smp
from vtk.util import numpy_support

# Input files
trajectory_file        = '/path/to/trajectory.vtp'
road_marking_file      = '/path/to/roadmarking.csv'
aggregated_frames_file = '/path/to/aggregated_frames.vtp'

# Output files (leave empty to ignore snapshots saving)
frames_out_dir = '/path/to/road_marking_detection/images/'

# Intensity colormap bounds
colormap_bounds = (0, 255)

# Tuple : (axes, values in degrees)
camera_to_lidar_rotation = 'XYZ', [0, 0, 0]

# Animation time management
animation_start_timestep = 0  # which timestep to begin animation at
animation_stop_timestep = -1  # which timestep to end animation at (negative value possible)
animation_nb_steps = 100      # number of timesteps to sample from start to stop

################################################################################
# Misc display settings
################################################################################

# get active view
render_view = smp.GetActiveViewOrCreate('RenderView')

# set black background
render_view.BackgroundColorMode = "Gradient"
render_view.UseColorPaletteForBackground = False
render_view.Background = [0.0, 0.0, 0.0]

# get color transfer function/color map for 'intensity'
intensityLUT = smp.GetColorTransferFunction('intensity')
intensityLUT.RGBPoints = [0.0, 0.0, 0.0, 1.0, 100.0, 1.0, 1.0, 0.0, 255.0, 1.0, 0.0, 0.0]
intensityLUT.ColorSpace = 'HSV'
intensityLUT.RescaleTransferFunction(*colormap_bounds)

# get color transfer function/color map for 'Time'
timeLUT = smp.GetColorTransferFunction('Time')

# Helper to simplify default display in the global render_view
def DisplayPoints(polydata, representation='Surface', hide=False,
                  color_array=None, color_lut=None, display_colorbar=False,
                  point_size=None, line_width=None):
  """ Helper to simplify default display in the global `render_view` """
  display = smp.Show(polydata, render_view)
  display.Representation = representation
  # Set colormap
  if color_array:
    display.ColorArrayName = ['POINTS', color_array]
    display.LookupTable = color_lut if color_lut else smp.GetColorTransferFunction(color_array)
    display.SetScalarBarVisibility(render_view, display_colorbar)
  # Set point/line size
  if point_size:
    display.PointSize = point_size
  if line_width:
    display.LineWidth = line_width
  # Hide entry if requested (useful to preset some display settings)
  if hide:
    smp.Hide(polydata, render_view)
  # Update the view to ensure updated data information
  render_view.Update()
  return display

################################################################################
# Trajectory
################################################################################

# Read trajectory
trajectory = smp.XMLPolyDataReader(FileName=[trajectory_file])
trajectory.UpdatePipeline()
smp.RenameSource('trajectory', trajectory)

# Get trajectory timesteps
trajectory_timesteps = numpy_support.vtk_to_numpy(trajectory.GetClientSideObject().GetOutput().GetPointData().GetArray('Time')).copy()

# Display trajectory using time coloring
trajectory_display = DisplayPoints(trajectory, 'Time')
timeLUT.RescaleTransferFunction(trajectory_timesteps[0], trajectory_timesteps[-1])

################################################################################
# Aggregated frames
################################################################################

# Load aggregated pointcloud
aggregated_frames = smp.XMLPolyDataReader(FileName=[aggregated_frames_file])
aggregated_frames.PointArrayStatus = ['intensity']
smp.RenameSource('aggregated_frames', aggregated_frames)

# Display pointcloud using intensity coloring
aggregated_frames_display = DisplayPoints(aggregated_frames, 'intensity')

################################################################################
# Road marking
################################################################################

# Load roadmarking detection
road_marking_csv = smp.CSVReader(FileName=[road_marking_file])
smp.RenameSource('road_marking_csv', road_marking_csv)

# Transform to pointcloud
road_marking = smp.TableToPoints(Input=road_marking_csv)
road_marking.XColumn = 'x'
road_marking.YColumn = 'y'
road_marking.ZColumn = 'z'
smp.RenameSource('road_marking', road_marking)

# Display road marking detections
road_marking_display = DisplayPoints(road_marking, point_size=3)

################################################################################
# Animation
################################################################################

# Create an animation cue with temporal_animation_cue_helpers
anim_cue = smp.PythonAnimationCue()
anim_cue.Script = """
from scipy.spatial.transform import Rotation
import camera_path as cp
import temporal_animation_cue_helpers as tach

# variables setup
tach.params['trajectory_name'] = "trajectory"
tach.params['frames_output_dir'] = "{frames_out_dir}"

cp.R_cam_to_lidar = Rotation.from_euler(*{R_cam_to_lidar}, degrees=True)

def start_cue(self):
    tach.start_cue_generic_setup(self)

    # third person view
    c0 = cp.ThirdPersonView(0, 650, up_vector=[0, 0, 1], focal_point=[0, 10, -1], position=[0, -10, 5])

    # top left view
    c1 = cp.ThirdPersonView(650, 1429, up_vector=[1, 0, 0.1], focal_point=[0, 0, 0], position=[-25, 0, 50])
    c1.set_transition(c0, 70, "s-shape")

    self.cameras = [c0, c1]

from temporal_animation_cue_helpers import tick, end_cue
""".format(frames_out_dir=frames_out_dir,
           R_cam_to_lidar=camera_to_lidar_rotation)

# Check animation timesteps bounds
nb_timesteps = len(trajectory_timesteps)
while animation_stop_timestep < 0:
  animation_stop_timestep = nb_timesteps + animation_stop_timestep
animation_start_timestep = min(nb_timesteps, max(0, animation_start_timestep))
animation_stop_timestep = min(nb_timesteps, max(0, animation_stop_timestep))

# Set animation times
animation = smp.GetAnimationScene()
animation.Cues.append(anim_cue)
animation.PlayMode = 'Sequence'
animation.NumberOfFrames = animation_nb_steps
animation.StartTime = trajectory_timesteps[animation_start_timestep]
animation.EndTime   = trajectory_timesteps[animation_stop_timestep]
animation.GoToFirst()

# ---- Play the animation
animation.Play()
