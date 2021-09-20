"""
This script is an example of how to easily write a PythonAnimationCue with
the temporal_animation_cue_helpers lib to follow a camera path along time.

You can run this script with the command:
`./install/bin/LidarView --script=${LV_SOURCES}/LVCore/Utilities/Animation/examples/example_temporal_animation.py`

In order to generate the video frm the images saved by this script, use:

```
ffmpeg  -i <out folder>/<pcap_file basename>/%06d.png -c:v libx264  -pix_fmt yuv420p -filter:v fps=00 <output video path> -loglevel error

```
It has been tested and tuned on the LiDAR dataset from la-doua-22-03.
"""
import os
import paraview.simple as smp
import colormap_tools as cmt
import pipeline_setup_helpers as psh

################################################################################
#  Script parameters
################################################################################


#################
# Paths to update
#################
# Input LiDAR frames and trajectory files
pcap_file  = '/data/tmp/hesai_demo/128_OPENROAD.pcap'
calib_file = '/data/tmp/hesai_demo/angle_correction_PandarXT.csv'
traj_file = '/data/tmp/hesai_demo/traj.poses'

# Path to where the car model is stored, set to "" to skip displaying a car at
# the sensor location
car_model_path = "/data/local_data/3D_vehicle_models/small-red-pickup.obj"

# Path to directory where to save snapshots
# (leave empty '' if you don't need to save snapshots)
output_dir = "/data/tmp/hesai_demo/output/" + os.path.basename(pcap_file)
# output_dir = ""

hesai_plugin_path = "/data/Dev/lidarView/build/install/lib/libHesaiPlugin.so"


################
# Parametrs that might require a change depending on the deisred video output
################

resolution = [1920, 1080]

interpreter="Hesai Packet Interpreter"

# View parameters
use_eye_dome_lighting = False
use_gradient = 0
background = [0.0, 0.0, 0.0]
background2 = [0.0, 0.0, 0.2]

nb_trailing_frames = 100


interpreter="Hesai Packet Interpreter"
timestamp_array_name = "Timestamp"
trajectory_time_conversion_factor = 1

coloring = "Intensity"
colormap_preset = "rainbow"
color_scale = [0, 100.0]

# Remove points with distance closer than distance_min (in order to
# remove points that reflect on the car (for outdoor datasets)
# as the Threshold filter requires a range, set a maximun to a huge value
distance_min = 3
distance_max = 10000

# camera_params = {"name": "pseudo_first_person_view",  # Name for the view (not used if only one view)
#                  "focal_point": [0, 0, 0], # Focal point relative to the trajectory
#                  "position": [0, -5, 10],  # Position of the camera, relative to the trajectory
#                  "up_vector": [0, 0, 1],
#                  "delay_s": 0}

camera_params = {"name": "third person, up and back",  # Name for the view (not used if only one view)
                 "focal_point": [0, 15, 0], # Focal point relative to the trajectory
                 "position": [0, -15, 15],  # Position of the camera, relative to the trajectory
                 "up_vector": [0, 0, 1],
                 "delay_s": 0}


# Car model params
car_model_scale = [0.08, 0.08, 0.08]
car_model_translation = [0, 0, -2]
car_model_rotation = [90, 0, 180]
car_model_color = [0.53, 0, 0]

################################################################################
#  Script
################################################################################
smp.LoadPlugin(hesai_plugin_path, remote=False, ns=globals())

render_view = psh.setup_view(use_eye_dome_lighting=use_eye_dome_lighting,
                             use_gradient=use_gradient,
                             background=background,
                             background2=background2)

trajectory = psh.load_trajectory(traj_file)
lidar_reader = psh.load_lidar_frames(pcap_file, calib_file, interpreter)

# Remove closest points
threshold = smp.Threshold(Input=lidar_reader,
                          Scalars=['POINTS', 'Distance'],
                          ThresholdRange=[distance_min, distance_max])

lidar_reader = smp.ExtractSurface(Input=threshold)
lidar_reader.UpdatePipeline()

trailing_frame = psh.aggregate_frames(
        lidar_reader, trajectory,
        nb_frames=nb_trailing_frames,
        timestamp_array_name=timestamp_array_name,
        trajectory_conversion_factor= trajectory_time_conversion_factor)

intensity_lut = smp.GetColorTransferFunction(coloring)
intensity_lut.ApplyPreset(colormap_preset, True)
intensity_lut.RescaleTransferFunction(*color_scale)
trailing_frame_display = smp.Show(trailing_frame, render_view)
smp.ColorBy(trailing_frame_display, ("POINTS", coloring))
render_view.Update()

if car_model_path:
    car_model = psh.load_car_model(car_model_path,
                                   car_model_scale,
                                   car_model_translation,
                                   car_model_rotation)
    car_model_display = smp.Show(car_model)
    car_model_display.DiffuseColor = car_model_color

smp.SetActiveSource(trailing_frame)

animation = psh.setup_animation(trajectory,
                                output_dir,
                                resolution,
                                camera_params)

# animation.Play()
