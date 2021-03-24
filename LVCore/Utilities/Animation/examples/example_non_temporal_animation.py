"""
This script is an example of non-temporal animation.
It doesn't use Paraview animation mechanism, and isn't based on data timestamps.

This script moves the camera along an absolute orbit path, and saves snapshots
along trajectory.
"""
import os
import paraview.simple as smp
import camera_path as cp

# How many steps/frames during orbit
nb_frames = 100

# Directory where to save snapshots
frames_out_dir = "/mnt/ramdisk/images/"

# Example of camera path : orbit around absolute position
center      = [ 0.,  0.,  0.]
up_vector   = [ 0.,  0.,  1.]
initial_pos = [ 0., -5.,  2.]
focal_point = [ 0.,  0.,  0.]
c = cp.AbsoluteOrbit(0, nb_frames,
                     center=center,
                     up_vector=up_vector,
                     initial_pos = initial_pos,
                     focal_point=focal_point)

# Get render view and modify background gradient colors
view = smp.GetRenderView()
view.Background  = [0, 0, 0]
view.Background2 = [0, 0, 0.2]

for i in range(nb_frames):
    # Move camera
    # (absolute orbit automatically compute its position and orientation)
    view.CameraPosition = c.interpolate_position(i, None, None, None)
    view.CameraFocalPoint = c.interpolate_focal_point(i, None, None)
    view.CameraViewUp = c.interpolate_up_vector(i, None)

    # Update rendering
    smp.Render()

    # Save render view snapshot
    image_name = os.path.join(frames_out_dir, "%06d.png" % (i))
    smp.SaveScreenshot(image_name)
    print(i)
