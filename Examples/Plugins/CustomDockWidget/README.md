# Custom Dock Widget Example

This example showcase how to add a dock widget and a toolbar in ParaView/LidarView. It also makes a interface with a third-party library (`zmq` here).

In this exemple, it is assumed that zmq is built appart from LidarView and that the path to its cmake config file is provided in the cmake configuration step under the ZeroMQ_DIR variable

### File Architecture

| **File**                      | **Description**                                                              |
|-------------------------------|------------------------------------------------------------------------------|
| **CMakeLists.txt**            | Main entry point to build the plugin. It uses `paraview_add_plugin` macro to |
|                               | add custom qt interfaces and the DummyExternalAPI vtk module.                |
| **lqCustomDockWidget.cxx/.h** | C++ Qt code for the implementation of the dock widget example.               |
| **lqDockToolBar.cxx/.h**      | C++ Qt code for the implementation of the toolbar example.                   |
| **paraview.plugin**           | File that names the plugin and defines some required VTK modules.            |
| **Resources/**                | Folder that regroups Qt .ui files and other resources such as icons.         |

#### DummyExternalAPI/

| **File**                       | **Description**                                                            |
|--------------------------------|----------------------------------------------------------------------------|
| **CMakeLists.txt**             | Builds the interpreter                                                     |
| **vtk.module**                 | Defines all module dependencies (both private and public).                 |
| **vtkDummyExternalAPI.h/.cxx** | Utilities function to interface the external library with out plugin code  |
| **zmqInfo.h.in**               | Header configured by cmake to indicate if the build links with zmq library |
