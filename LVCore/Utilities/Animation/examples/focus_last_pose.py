"""
Focus camera on last pose of a given trajectory during live run.
This will only change camera's focal point and center of rotation, applying the
same relative camera pos offset than previous timestep's.
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

anim_cue = smp.PythonAnimationCue()
anim_cue.Script = """
import paraview.simple as smp
import numpy as np

def start_cue(self):
    # If selected object is a SLAM filter, use SLAM trajectory on output port 1
    src = smp.GetActiveSource().GetClientSideObject()
    self.trajectory = src.GetOutput(1 if src.GetClassName() == 'vtkSlam' else 0)

def tick(self):
    view = smp.GetActiveView()
    offset = np.asarray(view.CameraPosition) - np.asarray(view.CameraFocalPoint)
    traj_pos = np.asarray(self.trajectory.GetPoints().GetData().GetTuple3(self.trajectory.GetNumberOfPoints() - 1))
    view.CameraFocalPoint = traj_pos
    view.CenterOfRotation = view.CameraFocalPoint
    view.CameraPosition = traj_pos + offset

def end_cue(self):
  pass
"""

smp.GetAnimationScene().Cues.append(anim_cue)