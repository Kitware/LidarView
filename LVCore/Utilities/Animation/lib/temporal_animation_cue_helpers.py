""" This module helps you easily write Paraview PythonAnimationCue()

PythonAnimationCue() expects a python script provided as a string.
An existing way of providing such a script would be to create a python script
in the lidarview sources, and modify it every time we want to run an animation
cue with a different camera path.

This module provides helpers to help write minimal python script strings
directly in your main script in the case of a temporal animation.
(Temporal animations:
 - Animations that depend on the data.
 - They require providing a trajectory.
 - The only supported animation mode is "Snap To Timesteps".)

```python
import temporal_animation_cue_helpers as tach
import camera_path as cp

# tick and end_cue methods don't depend on the camera path so they can
# be directly imported from the temporal_animation_cue_helpers module
# if they are used with their default keyword parameters

tach.trajectory_name = "your-trajectory"
tach. cad_model_name = "your-model"

def start_cue(self):
    tach.start_cue_generic_setup(self)
    c1 = cp.FirstPersonView(...)
    c2 = cp.FixedPositionView(...)
    self.cameras = [c1, c2]

from temporal_animation_cue_helpers import tick, end_cue
```

"""

import os
import math
import numpy as np
import paraview.simple as smp
from scipy.spatial.transform import Rotation
import camera_path as cp
from vtk.util import numpy_support


## --- Default values that can be / need to be overriden-----------------------
# Pipeline parameters.

# trajectory filiter/source name:
# points to be considered as the camera trajectory for the first person view
# (usually the vehicle trajectory). It must have the following PointData:
#   - Time (the time must be synced with the frames timesteps
#   - Orientation(AxisAngle)
trajectory_name = "firstPersonTrajectory"

# cad model filter/source name:
# Object mesh that is placed by the animation cue on the current point of the
# trajectory at each timestep (usually a vehicle)
cad_model_name = ""

# Camera / Lidar orientation (need to be specified)
# rotation which transforms the lidar tri-axe into the camera tri-axe

# example calibration ENS drone
# cp.R_cam_to_lidar = Rotation.from_euler('XYZ', [0.0, -90.0, 90.0], degrees=True)

# example calibration la doua car
cp.R_cam_to_lidar = Rotation.from_euler('ZYZ', [17, 90.0, -90.0], degrees=True)

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
        - setting the start timestep

    Example:
        def start_cue(self)
            tach.start_cue_generic_setup(self)
            c1 = ...
            c2 = ...
            self.cameras = [c1 c2]

    """

    self.image_index = 0	# image index used for files naming

    # setup view parameters
    view = smp.GetActiveView()

    # get trajectory positions and orientations
    trajectory = smp.FindSource(trajectory_name)
    traj = trajectory.GetClientSideObject().GetOutput()
    self.pts = numpy_support.vtk_to_numpy(traj.GetPoints().GetData()).copy()

    # convert lidarview axis angle to scipy Rotation
    orientations_data = traj.GetPointData().GetArray("Orientation(AxisAngle)")
    orientations = numpy_support.vtk_to_numpy(orientations_data).copy()
    axis = orientations[:, :3]
    angles = orientations[:, 3].reshape((-1, 1))
    axis_angles = axis * angles
    self.orientations = [Rotation.from_rotvec(a) for a in axis_angles]

    # get time data from trajectory
    time_data = traj.GetPointData().GetArray("Time")  # points times in secs
    timesteps = numpy_support.vtk_to_numpy(time_data).copy()

    # get the 3D model
    self.model = None
    if len(cad_model_name) > 1:
        self.model = smp.FindSource(cad_model_name)

    # get all available timesteps and find the index corresponding to the start time
    time = view.ViewTime
    self.i = np.argmin(np.abs(np.asarray(timesteps) - time))
    print "Start trajectory timestep: ", self.i

    self.cameras = []


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
    view = smp.GetActiveView()

    # lidar orientation and position
    R_l = self.orientations[self.i]
    T_l = self.pts[self.i, :]

    # move camera
    for c in self.cameras:
        if c.timestep_inside_range(self.i):
            print c.type
            view.CameraPosition = c.interpolate_position(self.i, R_l, T_l, np.asarray(list(view.CameraPosition)))
            view.CameraFocalPoint = c.interpolate_focal_point(self.i, R_l, T_l)
            view.CameraViewUp = c.interpolate_up_vector(self.i, R_l)
            break

    # move 3d model (vtk Transform rotation are angle axis in degrees)
    if self.model is not None:
        self.model.Transform.Translate = self.pts[self.i, :3]
        o = R_l.as_rotvec()
        angle_rad = np.linalg.norm(o)
        angle_deg = np.rad2deg(angle_rad)
        self.model.Transform.Rotate = o * angle_deg / angle_rad

    smp.Render()

    # save frame
    if len(frames_output_dir) > 0:
        imageName = os.path.join(frames_output_dir, filenameFormat % (self.image_index))
        smp.SaveScreenshot(imageName, ImageResolution=imageResolution)

    self.image_index += 1
    self.i += 1


def end_cue(self):
    """ Function called at the end of an animation """
    print("End of the animation")
