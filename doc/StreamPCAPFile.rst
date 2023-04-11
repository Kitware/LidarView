.. _chapter:StreamPCAPFile:

Stream PCAP File
################

**LidarView** comes with a commond line utility that allows you to stream a
PCAP file with a saved sensor stream as if it were a live stream. This can be
usefully for a variety of reasons. If you have a very large PCAP file it can
be a challenge to open the whole file at once depending on the system resources.
It can be useful when exploring or demoing **LidarView**'s capabilities when
you do not have access to a live sensor stream.

PacketFileSender tool
=====================

On Linux and Windows installations of **LidarView** the command line tool to
steam files can be found in the ``bin`` directory of you installation. It is
called ``PacketFileSender`` on Linux and ``PacketFileSender.exe`` on Windows.
Basic usage for this tool is as follows along with options:

.. code-block:: bash

    Usage: PacketFileSender <pcap_file> [options]
    Allowed options:
    --help                          produce help message
    --ip arg (=127.0.0.1)           destination ip adress
    --loop                          run the capture in loop
    --lidarPort arg (=2368)         destination port for lidar packets
    --GPSPort arg (=8308)           destination port for GPS packets
    --speed arg (=1)                playback speed
    --display-frequency arg (=1000) print information after every interval of X
                                    sent packets

.. exercise:: Stream a PCAP File
   :label: StreamAPCAPFile
   :class: note

    Let's stream a saved sensor stream file. If you don't have access
    to PCAP file with a saved sensor stream you can download an example:
    CarLoop_VLP16_. Run the following command from a Windows command prompt (or
    a terminal if on Linux).

    ``C:\Program Files\LidarView 4.3.0\bin\PacketFileSender.exe CarLoop_VLP16.pcap``

    You should now be able to connect **LidarView** to a sensor stream running
    on port 2368 as is done in :numref:`OpenSaveSensorStream`.

.. _CarLoop_VLP16: https://drive.google.com/file/d/1eARfsQWMcAa34GBHfDOs1JQ7nazQM3Jo/view?usp=share_link>