import paraview.simple as smp
import os

# ---- Script parameters ------------------------------------------------------
lidarFilename = '/path/to/frame.vtp.series' #'TO SET'
trajectoryFilename = '/path/to/trajectory.vtp' #'TO SET'
animation_start_time = 0
animation_end_time = 100
timeshift = 0
framesOutDir = '/tmp/'

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

# Detect the ground
groundPlanDetector1 = smp.GroundPlanDetector(
    PointCloud=temporalTransformsApplier1,
    Trajectory=None
)
groundPlanDetector1.MinPoint = [2.0, -10.0, -5.0]
groundPlanDetector1.MaxPoint = [22.0, 10.0, 5.0]
groundPlanDetector1.PipelineTimeToLidarTime = timeshift
groundPlanDetector1.Trajectory = trajectoryReader


# ---- Setup data display -----------------------------------------------------
# show data in view
dataDisplay = smp.Show(temporalTransformsApplier1, renderView1)
trajectoryReaderDisplay = smp.Show(trajectoryReader, renderView1)
groundPlanDetector1Display = smp.Show(groundPlanDetector1, renderView1)
smp.ColorBy(groundPlanDeetector1Display, ('POINTS', 'intensity'))

renderView1.Update()


# ---- Add animation ----------------------------------------------------------
# Rename sources to have an easy name to use in the animation script
smp.RenameSource("lidarReader", lidarReader)
smp.RenameSource("trajectory", trajectoryReader)
cadModelName = ""

# Create an animation cue with temporal_animation_cue_helpers
anim_cue = smp.PythonAnimationCue()
anim_cue.Script = """
import temporal_animation_cue_helpers as tash
import camera_path as cp
from scipy.spatial.transform import Rotation

# variables setup
tash.temporal_source_name = "lidarReader"
tash.trajectory_name = "trajectory"
tash.cad_model_name = "{0}"
tash.frames_output_dir = "{1}"
cp.R_cam_to_lidar = Rotation.from_euler('ZYZ', [17, 90.0, -90.0], degrees=True)

# temporal cue creation
from temporal_animation_cue_helpers import tick, end_cue

def start_cue(self):
    tash.start_cue_generic_setup(self)
    c1 = cp.FirstPersonView(self.i, self.i+40, focal_point=[-1.0, 0.0, 5.0])
    c2 = cp.RelativeOrbit(self.i+50, self.i+80, up_vector=[0, 0, 1.0],
                          initial_pos = [0.0, -10, 10])
    c3 = cp.ThirdPersonView(self.i+90, self.i+100,
                            focal_point=[-1.0, 0.0, 5.0],
                            position=[2.0, -2.0, -10.0])

    c2.set_transition(c1, 10, "s-shape")
    c3.set_transition(c3, 10, "s-shape")
    self.cameras = [c1, c2, c3]

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



