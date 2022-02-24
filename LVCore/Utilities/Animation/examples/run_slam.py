"""
This script loads LiDAR pcap recording and runs SLAM on it to generate a 3rd person video.
You can run this script with the command:
`./install/bin/LidarView --script=${LV_SOURCES}/LVCore/Utilities/Animation/examples/run_slam.py`
"""
import os
from paraview.simple import *

# ===== Input files ============================================================

# Input files
pcap_file  = '/path/to/lidar/recording.pcap'
calib_file = '/path/to/lidar/calibration.xml'


# ===== Output options =========================================================

# Output dir
output_dir = "/path/to/output/directory/" + os.path.basename(pcap_file)

# Save SLAM maps and SLAM trajectory or snapshots and video ?
save_slam_outputs = True
save_snapshots_and_video = True

# ===== Pipeline options =======================================================

# SLAM parameters

# General parameters
slam_ego_motion_mode = 'Motion extrapolation'  # 'Motion extrapolation + Registration'
slam_undistortion_mode = 'Approximated'  # 'Disabled'

# LIDAR to BASE transform (XYZ, in meters/degrees)
slam_lidar_to_base_translation = [0, 0, 0]
slam_lidar_to_base_rotation    = [0, 0, 0]

# Keypoints extraction
slam_sske_neighborhood_width = 5
slam_sske_min_distance_to_sensor = 0.5
slam_sske_edge_min_intensity_gap = 50.0
slam_sske_edge_min_depth_gap = 0.30

# Maps parameters
slam_edge_map_resolution = 0.2
slam_plane_map_resolution = 0.5
slam_rolling_grid_dimension = 50
slam_rolling_grid_resolution = 10.0

# Trailing frames parameters (no TF if 0)
trailing_frames_number = 0

# Smooth SLAM trajectory for soft camera motion
smooth_slam_trajectory = False


# ===== Display options ========================================================

# Intensity colormap bounds
intensity_colormap_bounds = (0, 255)

# Show options
show_slam_frame = True
show_slam_trajectory = True
show_slam_maps = True


# ===== Animation parameters ===================================================

# Animation time management
animation_start_timestep = 0   # which timestep to begin animation at
animation_stop_timestep = 100  # which timestep to end animation at (negative value possible)
animation_step = 1             # run 1 frame over animation_step, as fast as possible (can be float value)
# WARNING : if you use animation_step != 1, trailing frames are messing with pipeline time and damaging SLAM
# You should set trailing_frames_number = 0 if using animation_step != 1.

resolution = (1920, 1080)  # 16:9 FHD
# resolution = (3840, 2160)  # 16:9 UHD


# ===== Third person view, relative to current SLAM pose =======================

# Parameters using local coordinates system : X = right, Y = front, Z = up
# Modify camera_to_lidar_rotation to your needs if you use another orientation 
camera_to_lidar_rotation = 'XYZ', [0, 0, 0]  # (axes, values in degrees)

# # > ~ First person view
# focal_point = [0,   0,   0]
# position    = [0, -40,  10]
# up_vector   = [0,   0,   1]

# > Behind top view
focal_point = [0,   0,   0]
position    = [0, -70,  40]
up_vector   = [0,   0,   1]

# > Top front left
# focal_point = [  0,  0,  0]
# position    = [-50, 20, 80]
# up_vector   = [  0,  0,  1]

# > Bird eye view
# focal_point = [0,  0,   0]
# position    = [0,  0, 100]
# up_vector   = [1,  0,   0]


################################################################################
# Output options check and completion
################################################################################

# Make output directory if it does not exist
if (save_slam_outputs or save_snapshots_and_video) and not os.path.isdir(output_dir):
    os.makedirs(output_dir)

# SLAM output files
slam_trajectory_file = os.path.join(output_dir, 'slam_trajectory.poses') if save_slam_outputs else ''
slam_edge_map_file   = os.path.join(output_dir, 'slam_edge_map.vtp')     if save_slam_outputs else ''
slam_plane_map_file  = os.path.join(output_dir, 'slam_plane_map.vtp')    if save_slam_outputs else ''

# Snapshots and video output (leave entry empty to ignore saving)
frames_dir = os.path.join(output_dir, 'images/')   if save_snapshots_and_video else ''
video_file = os.path.join(output_dir, 'video.mp4') if save_snapshots_and_video else ''

################################################################################
# Misc display settings
################################################################################

# Get active view and set black background
render_view = GetActiveViewOrCreate('RenderView')
render_view.BackgroundColorMode = "Single Color" # WIP Texture ?
render_view.UseTexturedBackground = 0
render_view.Background = [0.0, 0.0, 0.0]

# Set color transfer function/color map for 'intensity'
intensity_LUT = GetColorTransferFunction('intensity')
intensity_LUT.RGBPoints = [0.0, 0.0, 0.0, 1.0, 100.0, 1.0, 1.0, 0.0, 255.0, 1.0, 0.0, 0.0]
intensity_LUT.ColorSpace = 'HSV'
intensity_LUT.RescaleTransferFunction(*intensity_colormap_bounds)

# Helper to simplify default display in the global render_view
def DisplayPoints(polydata, representation='Surface', hide=False,
                  color_array=None, color_lut=None, display_colorbar=False,
                  point_size=None, line_width=None):
  """ Helper to simplify default display in the global `render_view` """
  display = Show(polydata, render_view)
  display.Representation = representation
  # Set colormap
  if color_array:
    display.ColorArrayName = ['POINTS', color_array]
    display.LookupTable = color_lut if color_lut else GetColorTransferFunction(color_array)
    display.SetScalarBarVisibility(render_view, display_colorbar)
  # Set point/line size
  if point_size:
    display.PointSize = point_size
  if line_width:
    display.LineWidth = line_width
  # Hide entry if requested (useful to preset some display settings)
  if hide:
    Hide(polydata, render_view)
  # Update the view to ensure updated data information
  render_view.Update()
  return display

################################################################################
# Measurement grid
################################################################################

# Get measurement grid
measurement_grid = FindSource('Measurement Grid')
measurement_grid.GridNbTicks = 30
measurement_grid.Origin = [0.0, 0.0, -2.0]
RenameSource('measurement_grid', measurement_grid)

# Hide grid
Hide(measurement_grid, render_view)

################################################################################
# LIDAR reader
################################################################################

# Load LiDAR frames
lidar_reader = LidarReader(FileName=pcap_file, CalibrationFile=calib_file)
lidar_reader.Interpreter = 'Velodyne Meta Interpreter'
lidar_reader.UpdatePipelineInformation()
RenameSource('lidar_reader', lidar_reader)


################################################################################
# SLAM
################################################################################

# create a new 'SLAM online'
slam = SLAMonline(PointCloud=lidar_reader, Calibration=OutputPort(lidar_reader, 1))
RenameSource('slam', slam)

trajectory_name = "slam"

# General parameters
slam.Outputcurrentkeypoints = 0
slam.Numberofthreads = 8
slam.EgoMotionmode = slam_ego_motion_mode
slam.Undistortionmode = slam_undistortion_mode

# BASE to LIDAR offset
slam.Translation = slam_lidar_to_base_translation
slam.Rotation    = slam_lidar_to_base_rotation

# Keypoints extraction
slam.Keypointsextractor = 'Spinning Sensor Keypoint Extractor'
slam.Keypointsextractor.Neighborhoodwidth = slam_sske_neighborhood_width
slam.Keypointsextractor.Mindistancetosensor = slam_sske_min_distance_to_sensor
slam.Keypointsextractor.Edgeminintensitygap = slam_sske_edge_min_intensity_gap
slam.Keypointsextractor.Edgemindepthgap = slam_sske_edge_min_depth_gap

# Maps parameters
slam.Edgesmapresolution = slam_edge_map_resolution
slam.Planesmapresolution = slam_plane_map_resolution
slam.Rollinggriddimension = slam_rolling_grid_dimension
slam.Rollinggridresolution = slam_rolling_grid_resolution

# SLAM display
slam_display_frame      = DisplayPoints(OutputPort(slam, 0), hide=not(show_slam_frame), color_array='intensity')
slam_display_trajectory = DisplayPoints(OutputPort(slam, 1), hide=not(show_slam_trajectory), line_width=4)
slam_display_edge_map   = DisplayPoints(OutputPort(slam, 2), hide=not(show_slam_maps), color_array='intensity', point_size=4)
slam_display_plane_map  = DisplayPoints(OutputPort(slam, 3), hide=not(show_slam_maps), color_array='intensity', point_size=3)


################################################################################
# Trailing frames
################################################################################

if trailing_frames_number > 0:
  # Aggregate the last trailing frames
  trailing_frame = TrailingFrame(Input=OutputPort(slam, 0))
  trailing_frame.NumberOfTrailingFrames = trailing_frames_number
  RenameSource("trailing_frame", trailing_frame)

  trailing_frame_display = DisplayPoints(trailing_frame, color_array='intensity')


################################################################################
# Smooth SLAM trajectory
################################################################################

if smooth_slam_trajectory:
  # Init MLS smoothing to filter out high-frequency motion
  trajectory_smoothing = MLSPosesSmoothing(Input=OutputPort(slam, 1))
  trajectory_smoothing.KernelRadius = 20
  trajectory_smoothing.PolynomeDegree = 3
  RenameSource("trajectory_smoothing", trajectory_smoothing)
  trajectory_name = "trajectory_smoothing"

  # It is necessary to show this trajectory, otherwise it won't be updated by
  # the pipeline, and camera won't move
  # However, using point gaussian representation will make it invisible
  trajectory_smoothing_display = DisplayPoints(trajectory_smoothing, representation='Point Gaussian')


################################################################################
# Animation
################################################################################

# Check animation timesteps
view_timesteps = GetTimeKeeper().TimestepValues
nb_view_timesteps = len(view_timesteps)
while animation_stop_timestep < 0:
  animation_stop_timestep += nb_view_timesteps
animation_stop_timestep = min(nb_view_timesteps - 1, animation_stop_timestep)
animation_nb_frames  = round((animation_stop_timestep - animation_start_timestep + 1) / animation_step)
animation_start_time = view_timesteps[animation_start_timestep]
animation_end_time   = view_timesteps[animation_stop_timestep]
fps = animation_nb_frames / (animation_end_time - animation_start_time)

# Create an animation cue with temporal_animation_cue_helpers
anim_cue = PythonAnimationCue()
anim_cue.Script = """
import numpy as np
from scipy.spatial.transform import Rotation
import camera_path as cp
import temporal_animation_cue_helpers as tach

cp.R_cam_to_lidar = Rotation.from_euler(*{camera_to_lidar_rotation}, degrees=True)

# Use last pose of the SLAM trajectory
tach.params['trajectory_name'] = "{trajectory_name}"
tach.params['trajectory_time_array'] = None

# Snapshots and video saving
tach.params['frames_output_dir'] = "{frames_out_dir}"
tach.params['image_resolution'] = {image_resolution}
tach.params['video_output_path'] = "{video_out_file}"
tach.params['video_framerate'] = {fps}

def start_cue(self):
  tach.start_cue_generic_setup(self)

  # If selected object is a SLAM filter, use SLAM trajectory on output port 1
  src = smp.FindSource(tach.params['trajectory_name']).GetClientSideObject()
  self.trajectory = src.GetOutput(1 if src.GetClassName() == 'vtkSlam' else 0)

  c = cp.ThirdPersonView(0, np.inf, focal_point={focal_point}, position={position}, up_vector={up_vector})
  self.cameras = [c]

  self.counter = 0

from temporal_animation_cue_helpers import tick

def end_cue(self):
  slam = smp.FindSource('slam')
  if "{slam_trajectory_file}":
    smp.SaveData("{slam_trajectory_file}", proxy=smp.OutputPort(slam, 1))
  if "{slam_edge_map_file}":
    smp.SaveData("{slam_edge_map_file}",   proxy=smp.OutputPort(slam, 2))
  if "{slam_plane_map_file}":
    smp.SaveData("{slam_plane_map_file}",  proxy=smp.OutputPort(slam, 3))
  tach.end_cue(self)

""".format(trajectory_name=trajectory_name,
           position=position, up_vector=up_vector, focal_point=focal_point,
           camera_to_lidar_rotation=camera_to_lidar_rotation,
           slam_trajectory_file=slam_trajectory_file, slam_edge_map_file=slam_edge_map_file, slam_plane_map_file=slam_plane_map_file,
           frames_out_dir=frames_dir,
           video_out_file=video_file,
           image_resolution=resolution,
           fps=fps)

# Add animation cue
animation = GetAnimationScene()
animation.Cues.append(anim_cue)
animation.StartTime = animation_start_time
animation.EndTime   = animation_end_time
if animation_step == 1:
  animation.PlayMode = 'Snap To TimeSteps'
else:
  animation.PlayMode = 'Sequence'
  animation.NumberOfFrames = animation_nb_frames
animation.GoToFirst()

# Play the animation
animation.Play()
