""" This module helps you easily write Paraview PythonAnimationCue()

PythonAnimationCue() expects a python script provided as a string.
An existing way of providing such a script would be to create a python script
in the lidarview sources, and modify it every time we want to run an animation
cue with a different camera path.

This module provides helpers to help write minimal python script strings
directly in your main script in the case of a temporal animation using an
already existing trajectory :
 - They require providing a trajectory that will be used to build camera path.
 - Animations that depend on a time field of the trajectory.

```python
import temporal_animation_cue_helpers as tach
import camera_path as cp

# tick and end_cue methods don't depend on the camera path so they can
# be directly imported from the temporal_animation_cue_helpers module
# if they are used with their default keyword parameters

tach.trajectory_name = "your-trajectory"
tach.cad_model_name = "your-model"

def start_cue(self):
    tach.start_cue_generic_setup(self)
    c1 = cp.FirstPersonView(...)
    c2 = cp.FixedPositionView(...)
    self.cameras = [c1, c2]

from temporal_animation_cue_helpers import tick, end_cue
```
"""
import os
import numpy as np
from scipy.spatial.transform import Rotation
import paraview.simple as smp
import camera_path as cp
from vtk.util import numpy_support


## --- Default values that can be / need to be overriden-----------------------
# Pipeline parameters.

# trajectory filter/source name:
# points to be considered as the camera trajectory for the first person view
# (usually the vehicle trajectory). It must have the following PointData arrays:
#   - Time (in seconds, must be synced with the frames timesteps)
#   - Orientation(AxisAngle) (axis angle representation, in radians)
trajectory_name = "firstPersonTrajectory"
trajectory_time_array = "Time"
trajectory_orientation_array = "Orientation(AxisAngle)"

# The time field of the trajectory must use the same time scale as the main view
# or animation time. If this is not your case, set this offset properly :
trajectory_to_animation_time_offset = 0.

# cad model filter/source name ("" to disable CAD model dispaly):
# Object mesh that is placed by the animation cue on the current point of the
# trajectory at each timestep (usually a vehicle)
cad_model_name = ""

# Camera / Lidar orientation (need to be specified)
# rotation which transforms the lidar tri-axe into the camera tri-axe
cp.R_cam_to_lidar = Rotation.from_euler('XYZ', [0, 0, 0], degrees=True)

# example calibration ENS drone
# cp.R_cam_to_lidar = Rotation.from_euler('XYZ', [0.0, -90.0, 90.0], degrees=True)

# example calibration la doua car
# cp.R_cam_to_lidar = Rotation.from_euler('ZYZ', [17, 90.0, -90.0], degrees=True)
# rotations around X, Y, Z are intrinsic rotations, hence in the case of
# dataset-la-doua, there is a first rotation around the Z axis (17 degrees) to
# compensate the lidar front orientation, then rotations around Y and Z
# ([0, 90, -90]) to have Z looking forward

# Output directory for the generated frames ("" to disable saving)
frames_output_dir = ""


# ----------------------------------------------------------------------------
def start_cue_generic_setup(self):
    """ This method runs genetic setup steps at cue start
    It is intended to be run inside a `start_cue` before the camera definition
    step.

    The different steps it runs are:
        - getting the trajectory
        - getting the frames orientations from trajectory
        - getting a 3D model (for example a car model to add to the frame display)
        - initializing snapshots saving
        - displaying the start timestep

    Example:
    ```python
    def start_cue(self)
        tach.start_cue_generic_setup(self)
        c1 = ...
        c2 = ...
        self.cameras = [c1, c2]
    ```
    """
    # get trajectory
    trajectory_source = smp.FindSource(trajectory_name)
    trajectory = trajectory_source.GetClientSideObject().GetOutput()

    # get positions
    points_data = trajectory.GetPoints().GetData()
    self.pts = numpy_support.vtk_to_numpy(points_data).copy()

    # get rotations and convert lidarview axis angle to scipy Rotation
    orientations_data = trajectory.GetPointData().GetArray(trajectory_orientation_array)
    orientations = numpy_support.vtk_to_numpy(orientations_data).copy()
    axis = orientations[:, :3]
    angles = orientations[:, 3, np.newaxis]
    axis_angles = axis * angles
    self.orientations = [Rotation.from_rotvec(a) for a in axis_angles]

    # get time data from trajectory
    time_data = trajectory.GetPointData().GetArray(trajectory_time_array)
    self.timesteps = numpy_support.vtk_to_numpy(time_data).copy()

    # get the 3D model
    self.model = smp.FindSource(cad_model_name) if cad_model_name else None

    # define cameras
    self.cameras = []

    # create output directory where to store snapshots
    if frames_output_dir and not os.path.isdir(frames_output_dir):
        os.makedirs(frames_output_dir)
    self.image_index = 0  # image index used for files naming

    # find the frame index corresponding to the start time
    animation_time = smp.GetAnimationScene().AnimationTime
    self.pose_idx = np.argmin(np.abs(self.timesteps + trajectory_to_animation_time_offset - animation_time))
    print "Start trajectory timestep: ", self.pose_idx


# ----------------------------------------------------------------------------
# TODO add decorator for tick in order to let the user choose the image
# filename format
def tick(self, filenameFormat="%06d.png", imageResolution=(1920, 1080)):
    """ Function called at each timestep

    As a PythonAnimationCue script expects a `tick` function with only a `self`
    argument, this function can be either directly used with its default kwargs
    or wrapped in another function that provides its kwargs.

    Ex:
    ```python
    import temporal_animation_cue_helpers as tach
    def tick(self):
        tach.tick(self, filenameFormat="...", imageResolution="...")
    ```
    """
    # get current time and corresponding frame
    animation_time = smp.GetAnimationScene().AnimationTime
    self.pose_idx = np.argmin(np.abs(self.timesteps + trajectory_to_animation_time_offset - animation_time))

    # lidar orientation and position
    R_l = self.orientations[self.pose_idx]
    T_l = self.pts[self.pose_idx, :]

    # move camera
    for c in self.cameras:
        if c.timestep_inside_range(self.pose_idx):
            print "frame", self.pose_idx, "time", animation_time, ":", c.type
            view = smp.GetActiveView()
            view.CameraPosition = c.interpolate_position(self.pose_idx, R_l, T_l, np.asarray(list(view.CameraPosition)))
            view.CameraFocalPoint = c.interpolate_focal_point(self.pose_idx, R_l, T_l)
            view.CameraViewUp = c.interpolate_up_vector(self.pose_idx, R_l)
            view.CenterOfRotation = view.CameraFocalPoint
            break

    # move 3d model (vtk Transform rotation are angle axis in degrees)
    if self.model:
        self.model.Transform.Translate = T_l
        self.model.Transform.Rotate = np.rad2deg(R_l.as_rotvec())

    # update rendering
    smp.Render()

    # save frame
    if frames_output_dir:
        imageName = os.path.join(frames_output_dir, filenameFormat % (self.image_index))
        smp.SaveScreenshot(imageName, ImageResolution=imageResolution)
    self.image_index += 1


# ----------------------------------------------------------------------------
def end_cue(self):
    """ Function called at the end of an animation """
    print("End of the animation")
