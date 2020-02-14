# Introduction

LidarView performs real-time visualization of live captured 3D LiDAR data
from Velodyne's HDL sensors (HDL-32E and HDL-64E).

LidarView can playback pre-recorded data stored in .pcap files. The HDL
sensor sweeps an array of lasers (32 or 64) 360&deg; and a vertical field of
view of 40&deg;/26&deg; with 5-20Hz and captures about a million points per
second (HDL-32E: ~700,000pt/sec; HDL-64E: ~1.3Million pt/sec).
LidarView displays the distance measurements from the HDL as point cloud
data and supports custom color maps of multiple variables such as
intensity-of-return, time, distance, azimuth, and laser id. The data can
be exported as XYZ data in CSV format or screenshots of the currently
displayed point cloud can be exported with the touch of a button.

# Features

-   Input from live sensor stream or recorded .pcap file
-   Visualization of LiDAR returns in 3D + time including 3d position
    and attribute data such as timestamp, azimuth, laser id, etc
-   Spreadsheet inspector for LiDAR attributes
-   Record to .pcap from sensor
-   Export to CSV or VTK formats
-   Record and export GPS and IMU data (*New in 2.0*)
-   Ruler tool (*New in 2.0*)
-   Visualize path of GPS data (*New in 2.0*)
-   Show multiple frames of data simultaneously (*New in 2.0*)
-   Show or hide a subset of lasers (*New in 2.0*)

# How to Obtain

No binary installers are provided yet (that should change in 2020 Q2), so people need to compile it. Keep an eye on this page: [Paraview Lidar](https://www.paraview.org/lidarview/).
The source code for LidarView is made available under the Apache 2.0 license.

# How to use

For "sensor streaming" (live display of sensor data) it
is important to change the network settings of the Ethernet adapter
connected to the sensor from automatic IP address to manual IP address
selection and choose:

In order for sensor streaming to work properly, it is important to
disable firewall restrictions for the Ethernet port. Disable the
firewall completely for the ethernet device connected to the sensor or
explicitly allow data from that Ethernet port of (including both public
and private networks).

When opening pre-recorded data or live sensor streaming data one is
prompted to choose a calibration file.

# How to build

Detailed instructions for building and packaging are available in the
[LidarView Developer Guide](Documentation/LidarView_Developer_Guide.md) .
