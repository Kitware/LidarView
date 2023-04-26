"""
    This module contains high level helper functions wrapping paraview.simple
    elements in order to setup a lidarview pipeline in the lidarview python
    console.
    It is intended to make scripting for animations generation easier

    WARNING: Those helpers are provided only as examples and don't cover all
    use-cases.
"""

import os
import paraview.simple as smp
import colormap_tools as cmt


def setup_view(use_eye_dome_lighting=False,
               use_gradient=False,
               background=[0., 0., 0.],
               background2=[0., 0., 0.2]):
    """
    Initial view setup, with:
        - preset background
        - optional use of eye dome lighting

    Default background is a uniform black background (for reduced screenshot
    file weight)
    """
    smp._DisableFirstRenderCameraReset()

    # setup view parameters
    render_view = smp.GetActiveViewOrCreate('RenderView')
    mes_grid = smp.FindSource('Measurement Grid')
    if mes_grid is not None:
        smp.Hide(mes_grid, render_view)

    if use_eye_dome_lighting:
        # a renderview is automatically created when opening Lidarview
        # hence it has to be deleted when replacing with a RenderViewWithEDL
        smp.Delete(render_view)
        del render_view
        render_view = smp.GetActiveViewOrCreate('RenderViewWithEDL')

    if use_gradient:
      render_view.BackgroundColorMode = "Gradient"
      render_view.UseColorPaletteForBackground = False
      render_view.Background  = background
      render_view.Background2 = background2

    render_view.CameraParallelProjection = 0
    render_view.CameraViewAngle = 60

    return render_view


def load_trajectory(trajectory_filename):
    """
    Get trajectory from file

    When using a `.poses` input file, please make sure that the format
    corresponds to the one read by the `TemporalTransformsReader` source.
    If left empty, no smoothing is applied.


    """
    # Load trajectory
    if trajectory_filename.lower().endswith(".vtp"):
        trajectory = smp.XMLPolyDataReader(FileName=[trajectory_filename])
    elif trajectory_filename.lower().endswith(".poses"):
        trajectory = smp.TemporalTransformsReader(FileName=trajectory_filename)
    else :
        raise ValueError("This scripts only accepts .vtp and .poses " +
                         "trajectories at the moment.")

    smp.RenameSource("trajectory", trajectory)
    trajectory.UpdatePipeline()

    return trajectory


def load_lidar_frames(lidar_filename, calib_filename='',
                      interpreter='Velodyne Meta Interpreter'):
    """
    Load lidar data either from
        - a pcap file
        - or a .series files containing the times for each frame of a
        series of .vtp files

    If providing a .pcap filename, the following additionnal fields must
    be filled:
        - calib_filename: the calibration file for the lidar device used for
        the acquisition
        - interpreter: A string corresponding to the data interpreter
    """
    if lidar_filename.lower().endswith('pcap'):
        lidar_reader = smp.LidarReader(guiName='Data',
                                       FileName=lidar_filename,
                                       CalibrationFile=calib_filename,
                                       Interpreter=interpreter)

    elif lidar_filename.lower().endswith('series'):
        lidar_reader = smp.XMLPolyDataReader(FileName=[lidar_filename])

    else:
        raise ValueError("Unrecognized lidar filename extension " +
                         "(expected pcap or series)")

    smp.RenameSource("Data", lidar_reader)
    lidar_reader.UpdatePipelineInformation()
    lidar_reader.UpdatePipeline()

    return lidar_reader


def aggregate_frames(lidar_reader, trajectory,
                     color_by_categories=False,
                     nb_frames=100,
                     merge_frames=False,
                     timestamp_array_name='adjustedtime',
                     trajectory_conversion_factor=1e-6):
    """
    Aggregate frames in the world coordinates by:
    - transforming LiDAR frames to world coordinates using vehicle trajectory
    - applying a trailing frame

    The trajectory time is parametered by:
    - timestamp_array_name: the name of the array containing the trajectory
    points timestamps
    - trajectory_conversion_factor: the conversion factor from the trajectory
    timestamps to seconds

    In cas the flag "color_by_categories" is set, the input data is thresholded
    in order to remove points that have a category <= 0 (ie. not classified)
    """
    # Transform LiDAR frames to world coordinates using vehicle trajectory
    temporal_transforms_applier = smp.TemporalTransformsApplier(
        PointCloud=lidar_reader,
        Trajectory=trajectory)
    smp.RenameSource("temporalTransformsApplier", temporal_transforms_applier)
    temporal_transforms_applier.ConversionFactorToSecond = trajectory_conversion_factor
    temporal_transforms_applier.Array = ['POINTS', timestamp_array_name]
    temporal_transforms_applier.UpdatePipeline()

    if color_by_categories:
        # Keep only points with category from 1 to 500 in order to ignore
        # points with no category
        threshold = smp.Threshold(Input=temporal_transforms_applier)
        smp.RenameSource("threshold", threshold)
        threshold.Scalars = ['POINTS', 'category']
        threshold.ThresholdRange = [1.0, 500.0]
        threshold.UpdatePipeline()

        # Convert from UnstructuredGrid to PolyData
        extract_surface = smp.ExtractSurface(Input=threshold)
        smp.RenameSource("extractSurface", extract_surface)

    # Aggregate the last 100 trailing frames
    trailing_frame = smp.TrailingFrame(
            Input=extract_surface if color_by_categories
            else temporal_transforms_applier)
    trailing_frame.NumberOfTrailingFrames = nb_frames
    trailing_frame.UpdatePipeline()

    if merge_frames:
        merge_blocks = smp.MergeBlocks(Input=trailing_frame)
        trailing_frame = smp.ExtractSurface(Input=merge_blocks)

    smp.RenameSource("trailingFrame", trailing_frame)

    return trailing_frame


def load_car_model(car_model_path, car_model_scale, car_model_translation,
                   car_model_rotation):
    """
    Load cad model and place it at the expected position/orientatioin
    Args:
    - car_model_path: path to the file containing the model
    - car_model_scale: 3 values vector for scaling along x, y and z
    - car_model_translation:  3 values vector for translation along, x, y, and z
    - car_model_rotation: 3 values vector for the Euler angles in degrees
        around x, y z (applies after the translation)
    """
    # Load CAD car model
    car_model_source = smp.WavefrontOBJReader(FileName=car_model_path)

    # Scale and rotate the car model to correct initial pose
    car_model_tmp = smp.Transform(Input=car_model_source)
    car_model_tmp.Transform.Scale = car_model_scale
    car_model_tmp.Transform.Translate = car_model_translation
    car_model_tmp.Transform.Rotate = car_model_rotation

    # Add a transform to enable the script to move the car model along
    # trajectory
    car_model = smp.Transform(Input=car_model_tmp)
    smp.RenameSource("carModel", car_model)
    car_model.UpdatePipeline()

    return car_model


def setup_colormaps(coloring, intensity_colormap_preset="rainbow",
                    cat_config_path="",
                    color_scale=[0, 100.0]):
    """
    Setup colormaps when coloring by intensity or category
    """
    if coloring == "intensity":
        intensity_lut = smp.GetColorTransferFunction('intensity')
        intensity_lut.ApplyPreset(intensity_colormap_preset, True)
        intensity_lut.RescaleTransferFunction(*color_scale)

    elif coloring == "category":
        cmt.colormap_from_categories_config(cat_config_path, 'category')

    else:
        raise ValueError("Unknown coloring. " +
                         "Expected 'intensity' or 'category'")


def setup_animation(trajectory,
                    output_dir,
                    resolution,
                    camera_params,
                    camera_to_lidar_rotation=('XYZ', [0, 0, 0]),
                    animation_start_step=0,
                    animation_end_step=-1,
                    traj_smoothing_params={},
                    source_matching_trajectory=None,
                    save_all_views=False):
    """
    Setup an animation following the trajectory and save screenshots
    Args:
    - trajectory: polydata object containing the trajectory
    - output_dir: path to the folder containing the output screenshots
    - resolution: resolution of the output screenshots
    - camera_params: camera parameters dict, for example:
            {name: "first_person_view",  # Name of the view, used as output folder name
             focal_point: [0, 0, 0],  # Focal point of the camera, relative to the trajectory
             position: [0, -2, 0.2],  # Position of the camera, relative to the trajectory
             up_vector: [0, 0, 1],  # Up vector of the camera
             delay_s: 60  # delay on the camera position alongside the trajectory, in seconds}
    - camera_to_lidar_rotation: tuple (axes, rotation angles in degrees)
    - animation_start_step: step index of the animation start (relative to
        animation.GetTimeKeeper.TimestepValues)
    - animation_end_step: step index of the animation end
    - traj_smoothing_params: optional dict for smoothing params:
        traj_smoothing_params should be a dictionary with the following keys:
        - use_mls_smoothing: do use MLS smoothing?
        - kernel_radius: size of the kernel rectangular function used for MLS
        - polynome_degree: degree of the polynomial parametric function
        See documentation for the MLSPosesSmoothing filter for more details
    - source_matching_trajectory: In case there are several sources in the
        pipeline, please provide a source object (ie. a pipeline element with a
        `TimestepValues` field) that has the same timesteps as the trajectory
        is expected to have
    - save_all_views: if True, save all the views in the snapshots instead of
        only the active one.

    """
    if traj_smoothing_params.get("use_mls_smoothing", False):
        assert ("kernel_radius" in traj_smoothing_params.keys())
        assert ("polynome_degree" in traj_smoothing_params.keys())
        mls_smoothing = smp.MLSPosesSmoothing(Input=trajectory)
        mls_smoothing.KernelRadius = traj_smoothing_params["kernel_radius"]
        mls_smoothing.PolynomeDegree = traj_smoothing_params["polynome_degree"]

        trajectory = mls_smoothing

    smp.RenameSource("camera_trajectory", trajectory)

    if source_matching_trajectory is None:
        # Compute time offset between view (data) and trajectory timesteps
        ref_timesteps = smp.GetTimeKeeper().TimestepValues
    else:
        ref_timesteps = source_matching_trajectory.TimestepValues

    trajectory_timesteps_bounds = trajectory.GetPointDataInformation().GetArray('Time').GetRange()
    time_offset = ref_timesteps[0] - trajectory_timesteps_bounds[0]
    time_offset += camera_params["delay_s"]
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Create an animation cue with temporal_animation_cue_helpers
    anim_cue = smp.PythonAnimationCue()
    anim_cue.Script = """
from scipy.spatial.transform import Rotation
import temporal_animation_cue_helpers as tach
import camera_path as cp
import numpy as np

# Third person view, relative to current pose
position    = {position}
up_vector   = {up_vector}
focal_point = {focal_point}
cp.R_cam_to_lidar = Rotation.from_euler(*{camera_to_lidar_rotation},
                                        degrees=True)

# variables setup
tach.params['cad_model_name'] = "carModel"
tach.params['trajectory_name'] = "camera_trajectory"
tach.params['trajectory_to_animation_time_offset'] = {time_offset}

tach.params['frames_output_dir'] = "{frames_out_dir}"
tach.params['filename_format'] = "%06d.png"
tach.params['image_resolution'] = {image_resolution}
tach.params['save_all_views_in_layout'] = {save_all_views}


def start_cue(self):
    tach.start_cue_generic_setup(self)

    c = cp.ThirdPersonView(0, np.inf, position=position,
                           focal_point=focal_point, up_vector=up_vector)

    self.cameras = [c]

from temporal_animation_cue_helpers import tick, end_cue

    """.format(position=camera_params["position"],
               up_vector=camera_params["up_vector"],
               image_resolution=tuple(resolution),
               focal_point=camera_params["focal_point"],
               time_offset=time_offset,
               frames_out_dir=output_dir,
               camera_to_lidar_rotation=camera_to_lidar_rotation,
               save_all_views=save_all_views
               )

    # Add animation cue
    animation = smp.GetAnimationScene()
    # clear animation cues
    # but keep the first element, which should be the time keeper
    animation.Cues = [animation.Cues[0]]
    animation.Cues.append(anim_cue)

    # Set animation times
    timesteps = animation.TimeKeeper.TimestepValues
    nb_frames = len(timesteps)
    animation.StartTime = timesteps[max(0, animation_start_step)]
    animation.EndTime = timesteps[min(nb_frames - 1, animation_end_step)]
    animation.PlayMode = 'Snap To TimeSteps'

    return animation

