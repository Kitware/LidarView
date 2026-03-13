# Threshold from file Example

`vtkThresholdFromFile` reads values from a CSV file, selects a specific array within the input, and includes or excludes any points whose values match those in the CSV.


### File Architecture

| **File**            | **Description**                                                            |
|---------------------|----------------------------------------------------------------------------|
| **CMakeLists.txt**  | Main entry point to build the plugin. It uses `paraview_add_plugin` macro. |
| **paraview.plugin** | File that names the plugin and defines some required VTK modules.          |

#### ThresholdFromFilePlugin/

| **File**                              | **Description**                                                                   |
|---------------------------------------|-----------------------------------------------------------------------------------|
| **CMakeLists.txt**                    | Builds the interpreter and add XML proxy file to ParaView server manager.         |
| **ThresholdFromFilePlugin.xml**       | Define the ParaView proxy properties and build the ui based on theses declaration |
| **vtk.module**                        | Defines all module dependencies (both private and public).                        |
| **vtkThresholdFromFilePlugin.h/.cxx** | VTK implementation of the filter.                                                 |
