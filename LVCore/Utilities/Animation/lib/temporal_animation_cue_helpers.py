""" This module helps you easily write Paraview PythonAnimationCue()

PythonAnimationCue() expects a python script provided as a string.
An existing way of providing such a script would be to create a python script
in the lidarview sources, and modify it every time we want to run an animation
cue with a different camera path.

This module provides helpers to help write minimal python script strings
directly in your main script in the case of a temporal animation using an
already existing trajectory:
 - They require providing a trajectory that will be used to build camera path.
 - Animations that depend on a time field of the trajectory.

```python
import temporal_animation_cue_helpers as tach
import camera_path as cp

# tick and end_cue methods don't depend on the camera path so they can
# be directly imported from the temporal_animation_cue_helpers module
# if they are used with their default keyword parameters

tach.params['trajectory_name'] = "your-trajectory"
tach.params['cad_model_name'] = "your-model"

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
from vtk.util import numpy_support

#===============================================================================
#   Parameters
#===============================================================================

params = {}

# ----------------------------------------------------------------------------
# Trajectory filter/source

# Poses to be considered as the camera trajectory for the relative views
# (usually the vehicle trajectory).
params['trajectory_name'] = "trajectory"

# Name of the PointData array encoding pose orientation
# (axis angle representation XYZ-A, A in radians)
params['trajectory_orientation_array'] = "Orientation(AxisAngle)"

# Name of the PointData array encoding pose timestamp (in seconds)
# If None/"", the last pose of the trajectory will be used. This can be useful
# to track a single-pose or on-the-fly-built trajectory
params['trajectory_time_array'] = "Time"

# If used, the time array of the trajectory must use the same time scale as the
# animation time. If this is not your case, set this offset properly.
params['trajectory_to_animation_time_offset'] = 0.

# ----------------------------------------------------------------------------
# CAD model filter/source

# Object mesh that is placed by the animation cue on the current point of the
# trajectory at each timestep (usually a vehicle)
# This input needs to be a Transform filter, originally placed at identity pose.
params['cad_model_name'] = ""  # "" to disable CAD model

# ----------------------------------------------------------------------------
# Snapshots options

# Output directory for the generated frames ("" to disable saving)
params['frames_output_dir'] = ""

# Snapshots resolution (width x height), in pixels
params['image_resolution'] = (1920, 1080)

# Save all views for the current layout? (If false, save only the current view)
params['save_all_views_in_layout'] = False

# Snapshots files names format
params['filename_format'] = "%06d.png"

# ----------------------------------------------------------------------------
# Video options

# Name of the video generated using snapshots ("" to disable generation)
params['video_output_path'] = ""

# Output video frame rate [fps]
params['video_framerate'] = 10.

# ----------------------------------------------------------------------------
# Other options

# Print some additional info at each step
params['verbose'] = True


#===============================================================================
#   Generic helpers
#===============================================================================

# ----------------------------------------------------------------------------
def get_pose_idx_from_time(trajectory_timesteps):
    """ Return the idx of the trajectory pose that best matches to current animation time """
    animation_time = smp.GetAnimationScene().AnimationTime
    trajectory_timesteps_offset = np.asarray(trajectory_timesteps) + params['trajectory_to_animation_time_offset']
    return np.argmin(np.abs(trajectory_timesteps_offset - animation_time))


# ----------------------------------------------------------------------------
def update_camera(camera, pose_idx, position, orientation):
    """ Update a camera path at a given step using current pose """
    if camera and camera.timestep_inside_range(pose_idx):
        view = smp.GetActiveView()
        view.CameraPosition = camera.interpolate_position(pose_idx, orientation, position, np.asarray(list(view.CameraPosition)))
        view.CameraFocalPoint = camera.interpolate_focal_point(pose_idx, orientation, position)
        view.CameraViewUp = camera.interpolate_up_vector(pose_idx, orientation)
        view.CenterOfRotation = view.CameraFocalPoint
        return True
    else:
        return False


# ----------------------------------------------------------------------------
def update_cad_model(model, position, orientation):
    """ Update 3D CAD model Transform filter pose """
    if model:
        # Update model pose (vtk Transform rotation are angle axis in degrees)
        model.Transform.Translate = position
        model.Transform.Rotate = np.rad2deg(orientation.as_rotvec())
        return True
    else:
        return False


# ----------------------------------------------------------------------------
def save_screenshot():
    """ Save screenshot to params['frames_output_dir'] if not empty """
    if params['frames_output_dir']:
        # Init images saving if not already done
        if not hasattr(save_screenshot, 'image_index'):
            save_screenshot.image_index = 0
            # Create output directory where to store snapshots
            if not os.path.isdir(params['frames_output_dir']):
                os.makedirs(params['frames_output_dir'])

        # Save screenshot
        image_name = os.path.join(params['frames_output_dir'], params['filename_format'] % (save_screenshot.image_index))
        if params['save_all_views_in_layout']:
            view_or_layout = smp.GetLayout()
        else:
            view_or_layout = smp.GetActiveView()

        smp.SaveScreenshot(image_name,
                           view_or_layout,
                           ImageResolution=params['image_resolution'])
        save_screenshot.image_index += 1
        return True
    else:
        return False


# ----------------------------------------------------------------------------
def generate_video():
    """ Merge snapshots into a video to params['video_output_path'] if not empty.
    Ensure ffmpeg is installed on your machine.
    """
    if params['video_output_path']:
        # Create output directory where to store video
        video_dir = os.path.dirname(params['video_output_path'])
        if not os.path.isdir(video_dir):
            os.makedirs(video_dir)

        # Run ffmpeg using system prompt
        images = os.path.join(params['frames_output_dir'], params['filename_format'])
        os.system("ffmpeg -framerate {} -i {} {}".format(params['video_framerate'], images, params['video_output_path']))
        return os.path.isfile(params['video_output_path'])
    else:
        return False

#===============================================================================
#   Animation cue methods
#===============================================================================

# ----------------------------------------------------------------------------
def start_cue_generic_setup(self):
    """ This method runs genetic setup steps at cue start
    It is intended to be run inside a `start_cue` before the camera definition
    step.

    The different steps it runs are:
        - getting the trajectory
        - getting a 3D model (for example a car model to add to the frame display)
        - initializing camera path

    Example:
    ```python
    def start_cue(self)
        tach.start_cue_generic_setup(self)
        c1 = ...
        c2 = ...
        self.cameras = [c1, c2]
    ```
    """
    # Get the trajectory
    traj_src = smp.FindSource(params['trajectory_name'])
    self.trajectory = traj_src.GetClientSideObject().GetOutput() if traj_src else None

    # Get the 3D model
    self.model = smp.FindSource(params['cad_model_name']) if params['cad_model_name'] else None

    # Reset cameras and camera path step index
    self.pose_idx = 0
    self.cameras = []


# ----------------------------------------------------------------------------
def tick(self):
    """ Function called at each timestep

    The different steps it runs are:
        - compute trajectory pose index corresponding to the current animation time
        - extract the pose from the trajectory at this index
        - update camera and model using updated pose
        - update render view
        - save screenshot
    """
    # Check that the trajectory source is valid
    if not self.trajectory:
        print("Trajectory source is invalid")
        return

    # If the time array needs to be used, compute the pose index corresponding
    # to the current animation time
    if params['trajectory_time_array']:
        time_data = self.trajectory.GetPointData().GetArray(params['trajectory_time_array'])
        timesteps = numpy_support.vtk_to_numpy(time_data)
        self.pose_idx = get_pose_idx_from_time(timesteps)
    # Otherwise, use the last pose
    else:
        self.pose_idx = self.trajectory.GetNumberOfPoints() - 1

    # Get the 'pose_idx'th position and orientation from the trajectory
    position = np.asarray(self.trajectory.GetPoints().GetData().GetTuple3(self.pose_idx))
    axis_angle = np.asarray(self.trajectory.GetPointData().GetArray(params['trajectory_orientation_array']).GetTuple4(self.pose_idx))
    orientation = Rotation.from_rotvec(axis_angle[:3] * axis_angle[3])

    # Update camera path
    for i, c in enumerate(self.cameras):
        if update_camera(c, self.pose_idx, position, orientation) and params['verbose']:
            print("time {}, pose {} : updating camera path {} of type '{}'".format(smp.GetAnimationScene().AnimationTime, self.pose_idx, i, c.type))

    # Move 3d model if necessary
    update_cad_model(self.model, position, orientation)

    # Update rendering
    smp.Render()

    # Save frame if necessary
    save_screenshot()


# ----------------------------------------------------------------------------
def end_cue(self):
    """ Function called at the end of an animation """
    # Generate video using snaphsots if necessary
    success = generate_video()
    if success and params['verbose']:
        print("Video saved to " + params['video_output_path'])

    if params['verbose']:
        print("End of the animation")
