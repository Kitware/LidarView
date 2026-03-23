import paraview.web.venv
import asyncio

from trame.app import get_server, asynchronous
from trame.widgets import vuetify, paraview
from trame.ui.vuetify import SinglePageLayout

from paraview import simple
from lidarview import simple as lvsmp

# -----------------------------------------------------------------------------
# trame setup
# -----------------------------------------------------------------------------

server = get_server(client_type = "vue2")
state, ctrl = server.state, server.controller

# -----------------------------------------------------------------------------
# ParaView code
# -----------------------------------------------------------------------------

intensityLUT = simple.GetColorTransferFunction('intensity')
intensityLUT.RGBPoints = [0.0, 0.0, 0.0, 1.0, 40.0, 1.0, 1.0, 0.0, 100.0, 1.0, 0.0, 0.0]
intensityLUT.ColorSpace = 'HSV'

stream = lvsmp.OpenSensorStream("VLP-16", "Velodyne")
stream.Start()
representation = simple.Show(stream)
simple.ColorBy(representation, ('POINTS', 'intensity'))

view = simple.GetRenderView()
view.UseColorPaletteForBackground = 0
view.Background = [0.0, 0.0, 0.2]
view.OrientationAxesVisibility = 0
view = simple.Render()

state.slam = None

# -----------------------------------------------------------------------------
# Callbacks
# -----------------------------------------------------------------------------

@state.change("play")
@asynchronous.task
async def update_play(**kwargs):
    while state.play:
        if lvsmp.RefreshStream(stream):
            ctrl.view_update_image()
            ctrl.view_update_geometry()
        await asyncio.sleep(0.1)

def on_slam_start():
    if state.slam:
        return
    state.slam = simple.SLAMonline(PointCloud=stream)
    simple.Hide(stream, view)
    for i in range(0, 7):
        slamDisplay = simple.Show(simple.OutputPort(state.slam, i), view)
        slamDisplay.Representation = 'Surface'

    view.Update()
    simple.SetActiveSource(state.slam)

def on_slam_reset():
    state.slam.Resetstate()

# -----------------------------------------------------------------------------
# GUI
# -----------------------------------------------------------------------------

state.trame__title = "LidarView"

with SinglePageLayout(server) as layout:
    layout.title.set_text("LiDAR Stream")
    layout.icon.click = ctrl.view_reset_camera

    with layout.toolbar:
        vuetify.VSpacer()

        vuetify.VCheckbox(
            v_model=("play", True),
            off_icon="mdi-play",
            on_icon="mdi-stop",
            hide_details=True,
            dense=True,
            classes="mx-2",
        )

        vuetify.VBtn(
            "Start a Slam",
            click=on_slam_start,
            color="primary",
            outlined=True,
        )

        vuetify.VBtn(
            "Reset Slam",
            click=on_slam_reset,
            color="primary",
            outlined=True,
        )

        with vuetify.VBtn(icon=True, click=ctrl.view_reset_camera):
            vuetify.VIcon("mdi-crop-free")

        vuetify.VProgressLinear(
            indeterminate=True,
            absolute=True,
            bottom=True,
            active=("trame__busy",),
        )

    with layout.content:
        with vuetify.VContainer(fluid=True, classes="pa-0 fill-height"):
            html_view = paraview.VtkRemoteLocalView(view, namespace="view")
            ctrl.view_update = html_view.update
            ctrl.view_update_geometry = html_view.update_geometry
            ctrl.view_update_image = html_view.update_image
            ctrl.view_reset_camera = html_view.reset_camera


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    server.start()
