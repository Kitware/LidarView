"""
Attach a fixed camera to the last pose of a given trajectory during live run.
Example :
- open a lidar pcap
- apply SLAM Online filter on the Data/Frame entry
- select the SLAM trajectory in pipeline browser
- run this script (tip: save it as macro to easily use it)
- hit Play : camera should follow last SLAM pose.

Tip : Raw trajectory output by SLAM filter may oscillate a bit, leading to a noisy
motion of the tracking camera. To remove this noise, apply an MLSPosesSmoothing
filter to the SLAM trajectory, and use the output of this filter as tracked poses.
"""
import paraview.simple as smp

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

# ===== Where and how to save snapshots ========================================

# (leave frames_out_dir empty if you don't need to save snapshots)
frames_out_dir = ""
# frames_out_dir = "/home/user/data/videos/lv_slam_run/images/"

resolution = (1920, 1080)  # 16:9 FHD
# resolution = (3840, 2160)  # 16:9 UHD

################################################################################
# Animation
################################################################################

# Create an animation cue with temporal_animation_cue_helpers
anim_cue = smp.PythonAnimationCue()
anim_cue.Script = """
import numpy as np
from scipy.spatial.transform import Rotation
import camera_path as cp
import temporal_animation_cue_helpers as tach

# Third person view, relative to current SLAM pose
position    = {position}
up_vector   = {up_vector}
focal_point = {focal_point}
cp.R_cam_to_lidar = Rotation.from_euler(*{camera_to_lidar_rotation}, degrees=True)

# Use last pose of the selected trajectory
tach.params['trajectory_time_array'] = None

# Snapshots saving
tach.params['frames_output_dir'] = "{frames_out_dir}"
tach.params['filename_format'] = "%06d.png"
tach.params['image_resolution'] = {image_resolution}

# Misc
tach.params['verbose'] = False

def start_cue(self):
    tach.start_cue_generic_setup(self)

    # If selected object is a SLAM filter, use SLAM trajectory on output port 1
    src = smp.GetActiveSource().GetClientSideObject()
    self.trajectory = src.GetOutput(1 if src.GetClassName() == 'vtkSlam' else 0)

    c = cp.ThirdPersonView(0, np.inf, position=position, focal_point=focal_point, up_vector=up_vector)
    self.cameras = [c]

from temporal_animation_cue_helpers import tick, end_cue

""".format(position=position, up_vector=up_vector, focal_point=focal_point,
           camera_to_lidar_rotation=camera_to_lidar_rotation,
           frames_out_dir=frames_out_dir,
           image_resolution=resolution)

# Add animation cue
smp.GetAnimationScene().Cues.append(anim_cue)