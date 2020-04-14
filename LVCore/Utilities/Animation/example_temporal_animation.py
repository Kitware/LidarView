""" This script is an example of how to use a temporal animation cue on Lidar data.
It has been tested and tuned on the dataset from la-doua using the following command
./insall/bin/LidarView --script=${LV_SOURCES}/LVCore/Utilities/Animation/example_temporal_animation.py

"""

import paraview.simple as smp
import os
import colormap_tools as cmt

# ---- Script parameters ------------------------------------------------------
lidarFilename = '/path/to/frame.vtp.series'  # TO SET
trajectoryFilename = '/path/to/trajectory.vtp'  # TO SET

categoriesConfigPath = '/path/to/metadata/categories/coco_categories.yaml'  # TO SET if you want to color pointcloud
framesOutDir = '/tmp/animation_output/'

# Animation parameters
animation_start_time = 0
animation_end_time = -1
timeshift = -1521644706.95
# Tuple, (axes, values in degrees)
# Will be provided to a scipy Rotation.from_euler
camera2LidarRotation = ('ZYZ', [17, 90.0, -90.0])
# Rotations around X, Y, Z are intrinsic rotations, hence in the case of dataset-la-doua,
# there is a first rotation around the Z axis (17 degrees) to compensate the lidar front
# orientation, then rotations around Y and Z ([0, 90, -90]) to have Z looking forward

# 3D model parameters (set carModelPath to '' to disable)
carModelPath = '/path/to/small-red-pickup.obj'  # TO SET if you want to display a 3D model (otherwise set it to "")
carModelRotation = [90, 90 + 17, 0]
carModelScale = [0.05] * 3
carModelTranslation = [0, 0, -1.5]
# [90, 90, 0] for the model orientation + [0, 17, 0] for the lidar orientation compensation

# View parameters
CameraParallelProjection = 0
Background = [0.0, 0.0, 0.0]
Background2 = [0.0, 0.0, 0.2]
UseGradientBackground = 1
CameraViewAngle = 60
# ---- End script parameters --------------------------------------------------


if not os.path.isdir(framesOutDir):
    os.makedirs(framesOutDir)


# ---- Setup view -------------------------------------------------------------
smp._DisableFirstRenderCameraReset()
renderView1 = smp.GetActiveViewOrCreate('RenderView')

# setup view parameters
renderView1 = smp.GetActiveView()
renderView1.CameraParallelProjection = CameraParallelProjection
renderView1.Background = Background
renderView1.Background2 = Background2
renderView1.UseGradientBackground = UseGradientBackground
renderView1.CameraViewAngle = CameraViewAngle


# ---- LidarView pipeline -----------------------------------------------------
# Create readers
lidarReader = smp.XMLPolyDataReader(FileName=[lidarFilename])
trajectoryReader = smp.XMLPolyDataReader(FileName=[trajectoryFilename])
if not carModelPath:
    carModelSource = None
else:
    carModelSource = smp.WavefrontOBJReader(FileName=carModelPath)

lidarReader.UpdatePipeline()

# Apply Filters
# Apply trajectory to the point cloud
temporalTransformsApplier1 = smp.TemporalTransformsApplier(
    PointCloud=lidarReader,
    Trajectory=trajectoryReader
)
temporalTransformsApplier1.GetClientSideObject().DebugOn()
temporalTransformsApplier1.Array = ['POINTS', 'adjustedtime']
temporalTransformsApplier1.UpdatePipeline()

threshold1 = smp.Threshold(Input=temporalTransformsApplier1)
threshold1.Scalars = ['POINTS', 'category']
threshold1.ThresholdRange = [1.0, 500.0]
extractSurface1 = smp.ExtractSurface(Input=threshold1)

trailingFrame1 = smp.TrailingFrame(Input=extractSurface1)
trailingFrame1.NumberOfTrailingFrames = 1000

# Correct trajectory with lidar timesteps (which are the same as view timesteps)
# to have a common time base
correctedTraj = smp.PythonCalculator(
    Input=trajectoryReader,
    Expression='Time + {}'.format(-timeshift),
    ArrayName='Time'
)

if carModelSource is not None:
    # Scale and center the car model
    carModelTmp = smp.Transform(Input=carModelSource)
    carModelTmp.Transform.Scale = carModelScale
    carModelTmp.Transform.Translate = carModelTranslation
    carModelTmp.Transform.Rotate = carModelRotation
    # Add a transform to enable the script to move the car model
    carModel = smp.Transform(Input=carModelTmp)
else:
    carModel = None


# ---- Setup data display -----------------------------------------------------
# show data in view
trajectoryDisplay = smp.Show(correctedTraj, renderView1)
trajectoryDisplay = smp.Hide(correctedTraj, renderView1)

if carModel is not None:
    carModelDisplay = smp.Show(carModel)
    carModelDisplay.DiffuseColor = [0.53, 0.0, 0.0]

dataDisplay = smp.Show(trailingFrame1, renderView1)

categoryLut = cmt.colormap_from_categories_config(categoriesConfigPath)
smp.ColorBy(dataDisplay, ('POINTS', 'category'))

renderView1.Update()

smp.SetActiveSource(trailingFrame1)  # Trick to hide the widget box around the cad model

# ---- Add animation ----------------------------------------------------------
# Rename sources to have an easy name to use in the animation script
smp.RenameSource("lidarReader", lidarReader)
smp.RenameSource("trajectory", correctedTraj)
cadModelName = ""
if carModel is not None:
    smp.RenameSource("carModel", carModel)
    cadModelName = "carModel"

# Create an animation cue with temporal_animation_cue_helpers
anim_cue = smp.PythonAnimationCue()
anim_cue.Script = """
import temporal_animation_cue_helpers as tach
import camera_path as cp
from scipy.spatial.transform import Rotation

# variables setup
tach.trajectory_name = "trajectory"
tach.cad_model_name = "{0}"
tach.frames_output_dir = "{1}"
cp.R_cam_to_lidar = Rotation.from_euler("{2}", {3}, degrees=True)

# temporal cue creation
from temporal_animation_cue_helpers import tick, end_cue

def start_cue(self):
    tach.start_cue_generic_setup(self)
    # Pseudo first person
    # c1 = cp.FirstPersonView(self.i, self.i+40, focal_point=[0, 0, 1])
    c1 = cp.ThirdPersonView(self.i, self.i+40, focal_point=[0, 0, 20], position=[0, -1.7, -3.5])
    c2 = cp.ThirdPersonView(self.i+40, self.i+100, position=[0, -15, -30])
    c2.set_transition(c1, 5, "s-shape")		# transition from c1
    self.cameras = [c1, c2]

""".format(cadModelName, framesOutDir, camera2LidarRotation[0], camera2LidarRotation[1])

# Set animation times
animation = smp.GetAnimationScene()
animation.Cues.append(anim_cue)
animation.PlayMode = 'Snap To TimeSteps'
timesteps = animation.TimeKeeper.TimestepValues
nFrames = len(timesteps)

animation.StartTime = timesteps[max(0, animation_start_time)]
animation.EndTime = timesteps[min(nFrames-1, animation_end_time)]

# ---- Play the animation
animation.Play()



