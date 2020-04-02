import paraview.simple as smp
import os
import colormap_tools as cmt

# ---- Script parameters ------------------------------------------------------
lidarFilename = '/path/to/frame.vtp.series'  # TO SET
trajectoryFilename = '/path/to/trajectory.vtp'  # TO SET
animation_start_time = 0
timeshift = 0
framesOutDir = '/tmp/'
animation_end_time = -1
categoriesConfigPath = '/path/to/metadata/categories/coco_categories.yaml'  # TO SET if you want to color pointcloud
#by category

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

# Correct trajectory with lidar timesteps (which are the same as view timesteps)
# to have a common time base
correctedTraj = smp.PythonCalculator(
    Input=trajectoryReader,
    Expression='Time + {}'.format(-timeshift),
    ArrayName='Time'
)


# ---- Setup data display -----------------------------------------------------
# show data in view
dataDisplay = smp.Show(threshold1, renderView1)
#dataDisplay = smp.Show(lidarReader, renderView1)

trajectoryDisplay = smp.Show(correctedTraj, renderView1)

categoryLut = cmt.colormap_from_categories_config(categoriesConfigPath)
smp.ColorBy(dataDisplay, ('POINTS', 'category'))

renderView1.Update()


# ---- Add animation ----------------------------------------------------------
# Rename sources to have an easy name to use in the animation script
smp.RenameSource("lidarReader", lidarReader)
smp.RenameSource("trajectory", correctedTraj)

cadModelName = ""

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
cp.R_cam_to_lidar = Rotation.from_euler('ZYZ', [17, 90.0, -90.0], degrees=True)

# temporal cue creation
from temporal_animation_cue_helpers import tick, end_cue

def start_cue(self):
    tach.start_cue_generic_setup(self)
    c1 = cp.FirstPersonView(self.i, self.i+40, focal_point=[0, 0, 1])
    c2 = cp.FixedPositionView(self.i+40, self.i+100)
    c2.set_transition(c1, 5, "s-shape")		# transition from c1
    c3 = cp.AbsoluteOrbit(self.i+100, self.i+200,
            center=[99.65169060331509, 35.559305816556, 37.233268868598536],
	    up_vector=[0, 0, 1.0],
	    initial_pos = [85.65169060331509, 35.559305816556, 37.233268868598536],
	    focal_point=[99.65169060331509, 35.559305816556, 7.233268868598536])
    c3.set_transition(c2, 20, "s-shape")

    c4 = cp.ThirdPersonView(self.i+200, self.i+280)
    c4.set_transition(c3, 20, "s-shape")

    c5 = cp.RelativeOrbit(self.i+280, self.i+350, up_vector=[0, 0, 1.0], initial_pos = [0.0, -10, 10])
    c5.set_transition(c4, 20, "square")

#    c1 = cp.FirstPersonView(self.i, self.i+40)
#    c2 = cp.RelativeOrbit(self.i+40, self.i+80, up_vector=[0, 0, 1.0],
#                          initial_pos = [0.0, -10, 10])
#    c3 = cp.ThirdPersonView(self.i+80, self.i+100,
#                            position=[2.0, -2.0, -10.0])
#
#    c2.set_transition(c1, 10, "s-shape")
#    c3.set_transition(c3, 10, "s-shape")
    self.cameras = [c1, c2, c3, c4, c5]

""".format(cadModelName, framesOutDir)

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



