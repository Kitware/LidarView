# Time Based LiDAR Interpreter Example

This plugin showcase an implementation of a time based LiDARs.

### How to run it

- Build LidarView with the cmake option `-DBUILD_EXAMPLES=ON`
- Load manually the plugin in LidarView (Tools > Manage Plugins > TimeBasedLidarInterpreter > Load Selected)
- Open a new stream (File > Open Sensor Stream)
- Choose the TimeBasedLidarExample option for Interpreter Selection
- Start the python script `Utilities/point_sender.py`

### File Architecture

| **File**                             | **Description**                                                                     |
|--------------------------------------|-------------------------------------------------------------------------------------|
| **CMakeLists.txt**                   | Main entry point to build the plugin. It uses `paraview_add_plugin` macro.          |
| **paraview.plugin**                  | File that names the plugin and defines some required VTK modules.                   |
| **TimeBasedLidarExampleProxies.xml** | ParaView proxy used by LidarView to populate the open stream and open pcap dialogs. |
|                                      | Defines the name of interpreter to use.                                             |

#### TimeBasedLidarPacketInterpreter/

| **File**                                      | **Description**                                                                  |
|-----------------------------------------------|----------------------------------------------------------------------------------|
| **CMakeLists.txt**                            | Builds the interpreter and adds XML proxies file to ParaView server manager.     |
| **TimeBasedLidarFormat.h**                    | Defines the packet structure.                                                    |
| **TimeBasedLidarPacketInterpreter.xml**       | Instantiates interpreter proxy; some interpreter properties can be defined here. |
| **TimeBasedLidarReader.xml**                  | Defines the pcap reader proxy. Inherits properties from `CommonLidarReader`      |
|                                               | and uses `vtkLidarReader` by default, allowing for other implementations.        |
|                                               | Implements the SubProxy `PacketInterpreter` and exposes properties of interest.  |
| **TimeBasedLidarStream.xml**                  | Similar to `TimeBasedLidarReader.xml` but inherits from `CommonLidarStream`      |
|                                               | and uses `vtkLidarStream`.                                                       |
| **vtk.module**                                | Defines all module dependencies (both private and public).                       |
| **vtkTimeBasedLidarPacketInterpreter.h/.cxx** | C++ class implementation that inherits from `vtkLidarPacketInterpreter`.         |

#### Utilities/

| **File**            | **Description**                                               |
|---------------------|---------------------------------------------------------------|
| **point_sender.py** | A small script that emulates a LiDAR stream for this example. |
