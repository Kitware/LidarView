"""
Open a pre-set LiDAR recording using a specified calibration and interpreter.
"""
import paraview.simple as smp
import lidarview.simple as lvsmp

################################################################################
my_filename = '/home/user/data/lidar.pcap'
my_lidar_model = 'LIDAR-MODEL' # E.g VLP-16, HDL-32... for Velodyne
my_calibration = '/home/user/software/lidarview/build/install/share/calib.xml'
my_interpreter = 'My Interpreter' # E.g Velodyne, Hesai...
################################################################################

def LoadLidarData(filename, model, interpreter, calibration):
    # create a new 'Lidar Reader'
    data = lvsmp.OpenPCAP(filename, model, interpreter, CalibrationFileName=calibration)

    # get active view
    renderView1 = smp.GetActiveViewOrCreate('LidarGridView')

    # set active source
    smp.SetActiveSource(data)

    # get color transfer function/color map for 'intensity'
    intensityLUT = smp.GetColorTransferFunction('intensity')
    intensityLUT.RGBPoints = [0.0, 0.0, 0.0, 1.0, 40.0, 1.0, 1.0, 0.0, 100.0, 1.0, 0.0, 0.0]
    intensityLUT.ColorSpace = 'HSV'
    # get opacity transfer function/opacity map for 'intensity'
    intensityPWF = smp.GetOpacityTransferFunction('intensity')

    # show data in view
    dataDisplay = smp.Show(data, renderView1)
    dataDisplay.Representation = 'Surface'

    # set scalar coloring
    smp.ColorBy(dataDisplay, ('POINTS', 'intensity'))
    # rescale color and/or opacity maps used to include current data range
    dataDisplay.RescaleTransferFunctionToDataRange(True, False)
    # hide color bar/color legend
    dataDisplay.SetScalarBarVisibility(renderView1, False)

if __name__ == "__main__":
    LoadLidarData(my_filename, my_lidar_model, my_interpreter, my_calibration)
