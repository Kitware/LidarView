.. _chapter:BasicUsage:

Basic Usage
###########

Let us get started using |LidarView|. In order to follow along, you will
need your own installation of |LidarView|. If you do not already have |LidarView|,
you can download a copy from https://gitlab.kitware.com/LidarView/lidarview/-/releases.
|LidarView| launches like most other applications. On Windows, the
launcher is located in the start menu. On Macintosh, open the
application bundle that you installed. On Linux, execute ``LidarView`` from a
command prompt (you will need to set your path to point to where you unpacked
LidarView).

User Interface
==============

.. figure:: ./images/UserInterface.png

The |LidarView| GUI conforms to the platform on which it is running, but on
all platforms it behaves basically the same. The layout shown here is
the default layout given when |LidarView| is first started. The GUI
includes the following components in the toolbar at the top of the application.

Basic Controls
    This part of toolbar includes buttons to manage opening and saving files and
    to turn additional windows in the GUI such as the python console and the
    spreadsheet view.

Playback Controls
    These controls allow the user to temporally navigate the Lidar data. The
    data can be played through, advanced one frame at a time, or a specific
    time can be selected.

Color Controls
    These controls allow the user to select what data field determines how the
    point cloud is colored and adjust the color map used to represent the range
    of the selected data field.

View Controls
    These controls allow the user to set the prespective that the point cloud is
    viewed from.

Opening PCAP File
=================

There are two ways to get Lidar data into LidarView. You can connect to a sensor
steam or open a recorded sensor stream saved to a PCAP file. To open a PCAP file
into LidarView click on the Open Data File button |OpenPCAP| in the Basic
Controls section of the toolbar, select the PCAP file you wish to open and click
OK, and then select the appropriate sensor interpeter and click OK. You should
now see the first frame of the saved sensor data displayed in the main render
view.

.. |OpenPCAP| image:: ../Application/Ui/Widgets/images/WiresharkDoc-128.png
   :height: 20px

Playback Sensor Stream
======================

Now that we have loaded a saved sensor stream we can play it back. Click on the
|Play| button in the Playback Controls to have LidarView playback the sensor
stream. The playback can be paused at any time by clicking on the |Pause|
button which replaces the |Play| button while the stream is advancing.

.. |Play| image:: ../LVCore/ApplicationComponents/Icons/media-playback-start.png
   :height: 20px

.. |Pause| image:: ../LVCore/ApplicationComponents/Icons/media-playback-pause.png
   :height: 20px
