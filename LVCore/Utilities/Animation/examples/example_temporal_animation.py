"""
This script is an example of how to easily write a PythonAnimationCue with
the temporal_animation_cue_helpers lib to follow a camera path along time.

You can run this script with the command:
`./install/bin/LidarView --script=${LV_SOURCES}/LVCore/Utilities/Animation/examples/example_temporal_animation.py`

It has been tested and tuned on the LiDAR dataset from la-doua-22-03.
"""
import os
import paraview.simple as smp
import colormap_tools as cmt

################################################################################
#  Script parameters
################################################################################

# Input LiDAR frames and trajectory files
lidarFilename = '/path/to/frame.vtp.series'  # TO SET
trajectoryFilename = '/path/to/trajectory.vtp'  # TO SET

# Optional colormap for segmentation classes (or leave empty '' to colorize using intensity)
categoriesConfigPath = '/path/to/metadata/categories/coco_categories.yaml'  # TO SET

# Path to directory where to save snapshots
# (leave empty '' if you don't need to save snapshots)
framesOutDir = '/tmp/animation_output/images/'

# Animation parameters
animationStartTime = 0
animationEndTime = -1

# Tuple, (axes, values in degrees)
# Will be provided to a scipy Rotation.from_euler
camera2LidarRotation = ('ZYZ', [17, 90, -90])
# Rotations around X, Y, Z are intrinsic rotations, hence in the case of dataset-la-doua,
# there is a first rotation around the Z axis (17 degrees) to compensate the lidar front
# orientation, then rotations around Y and Z ([0, 90, -90]) to have Z looking forward

# 3D model parameters (set carModelPath to '' to disable)
carModelPath = '/path/to/small-red-pickup.obj'  # TO SET
carModelScale = [0.05] * 3
carModelTranslation = [0, 0, -1.5]
carModelRotation = [90, 90 + 17, 0]  # [90, 90, 0] for the model orientation + [0, 17, 0] for the lidar orientation compensation


################################################################################
#  Script
################################################################################

# ---- Setup view -------------------------------------------------------------

smp._DisableFirstRenderCameraReset()

# setup view parameters
renderView = smp.GetActiveViewOrCreate('RenderView')
renderView.BackgroundColorMode = "Gradient"
renderView.UseColorPaletteForBackground = False
renderView.Background = [0.0, 0.0, 0.0]
renderView.Background2 = [0.0, 0.0, 0.2]
renderView.CameraParallelProjection = 0
renderView.CameraViewAngle = 60

# ---- Trajectory -------------------------------------------------------------

# Load trajectory
trajectory = smp.XMLPolyDataReader(FileName=[trajectoryFilename])
smp.RenameSource("trajectory", trajectory)

# ---- LiDAR frames -----------------------------------------------------------

# Load LiDAR frames
lidarReader = smp.XMLPolyDataReader(FileName=[lidarFilename])
smp.RenameSource("lidarReader", lidarReader)
lidarReader.UpdatePipelineInformation()

# Transform LiDAR frames to world coordinates using vehicle trajectory
temporalTransformsApplier = smp.TemporalTransformsApplier(PointCloud=lidarReader,
                                                          Trajectory=trajectory)
smp.RenameSource("temporalTransformsApplier", temporalTransformsApplier)
temporalTransformsApplier.Array = ['POINTS', 'adjustedtime']

if categoriesConfigPath:
    # Keep only points with category from 1 to 500 in order to ignore points with no category
    threshold = smp.Threshold(Input=temporalTransformsApplier)
    smp.RenameSource("threshold", threshold)
    threshold.Scalars = ['POINTS', 'category']
    threshold.ThresholdRange = [1.0, 500.0]

    # Convert from UnstructuredGrid to PolyData
    extractSurface = smp.ExtractSurface(Input=threshold)
    smp.RenameSource("extractSurface", extractSurface)

# # Aggregate the last 100 trailing frames
trailingFrame = smp.TrailingFrame(Input=extractSurface if categoriesConfigPath else temporalTransformsApplier)
smp.RenameSource("trailingFrame", trailingFrame)
trailingFrame.NumberOfTrailingFrames = 100

# ---- Car CAD model ----------------------------------------------------------

if carModelPath:
    # Load CAD car model
    carModelSource = smp.WavefrontOBJReader(FileName=carModelPath)

     # Scale and rotate the car model to correct initial pose
    carModelTmp = smp.Transform(Input=carModelSource)
    carModelTmp.Transform.Scale = carModelScale
    carModelTmp.Transform.Translate = carModelTranslation
    carModelTmp.Transform.Rotate = carModelRotation

    # Add a transform to enable the script to move the car model along trajectory
    carModel = smp.Transform(Input=carModelTmp)
    smp.RenameSource("carModel", carModel)
else:
    carModel = None

# ---- Setup data display -----------------------------------------------------

# Show car model
if carModel is not None:
    carModelDisplay = smp.Show(carModel)
    carModelDisplay.DiffuseColor = [0.53, 0.0, 0.0]

# Show trailing frames
trailingFrameDisplay = smp.Show(trailingFrame, renderView)

# Load colormap from config file
if categoriesConfigPath:
    categoryLut = cmt.colormap_from_categories_config(categoriesConfigPath, 'category')
    smp.ColorBy(trailingFrameDisplay, ('POINTS', 'category'))
else:
    smp.ColorBy(trailingFrameDisplay, ('POINTS', 'intensity'))

renderView.Update()

smp.SetActiveSource(trailingFrame)  # Trick to hide the widget box around the cad model

# ---- Add animation ----------------------------------------------------------

# Compute time offset between view (data) and trajectory timesteps
view_timesteps = GetTimeKeeper().TimestepValues
trajectory_timesteps_bounds = trajectory.GetPointDataInformation().GetArray('Time').GetRange()
time_offset = view_timesteps[0] - trajectory_timesteps_bounds[0]

# Create an animation cue with temporal_animation_cue_helpers
anim_cue = smp.PythonAnimationCue()
anim_cue.Script = """
from scipy.spatial.transform import Rotation
import temporal_animation_cue_helpers as tach
import camera_path as cp

# variables setup
tach.params['cad_model_name'] = "carModel"
tach.params['trajectory_name'] = "trajectory"
tach.params['trajectory_to_animation_time_offset'] = {time_offset}
tach.params['frames_output_dir'] = "{frames_output_dir}"

cp.R_cam_to_lidar = Rotation.from_euler(*{R_cam_to_lidar}, degrees=True)

def start_cue(self):
    tach.start_cue_generic_setup(self)
    # Pseudo first person
    c1 = cp.ThirdPersonView( 0,  40, focal_point=[0, 0, 20], position=[0, -1.7, -3.5])
    c2 = cp.ThirdPersonView(40, 1000, position=[0, -15, -30])
    c2.set_transition(c1, 10, "s-shape")		# transition from c1
    self.cameras = [c1, c2]

from temporal_animation_cue_helpers import tick, end_cue

""".format(time_offset=time_offset,
           frames_output_dir=framesOutDir,
           R_cam_to_lidar=camera2LidarRotation)

# Add animation cue
animation = smp.GetAnimationScene()
animation.Cues.append(anim_cue)

# Set animation times
timesteps = animation.TimeKeeper.TimestepValues
nbFrames = len(timesteps)
animation.StartTime = timesteps[max(0, animationStartTime)]
animation.EndTime = timesteps[min(nbFrames - 1, animationEndTime)]
animation.PlayMode = 'Snap To TimeSteps'

# ---- Play the animation
animation.Play()
