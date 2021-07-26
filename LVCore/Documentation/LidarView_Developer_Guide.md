# LidarView Developer Guide
This documents gather all information needed to build LidarView-based Applications.

This document mirrors some information from the [Installation Guide](Documentation/INSTALLATION.md).

1. [Overview](#lv-compilation-overview)
2. [Configure & Build instructions](#congfigure-build)
    0. [Crossplatform](#crossplatform-instructions)
    1. [Windows](#windows-instructions)
    2. [Ubuntu](#linux-instructions)
3. [Enabling Additional Features](#enabling-additional-lidarview-features)
4. [Incremental Build Instructions](#incremental-build)
4. [Packaging Instructions](#packaging)
4. [Testing Instructions](#testing)
5. [Troubleshooting / FAQ ](#faq-instructions)

## LidarView Compilation Overview <a name="lv-compilation-overview"></a>
LidarView relies primarily on a CMake and the *superbuild* system, it handles the download and compilation of most, if not all, necessary dependencies.

However some dependencies, may or may not be supplied by the superbuild for various reasons and must obtained externally on some platform,
 see the following sections for per platform instructions.

LidarView is actively Maintained on Windows, Ubuntu18.04, Ubuntu20.04 and OSX.

## Configure and build instructions <a name="configure-build"></a>

### Crossplatform Instructions <a name="crossplatform-instructions"></a>

**Crossplatform Dependencies and Tools**

The specific version of the following tools may or may not be available in your OS package manager.

 - **[Required] CMake 3.18 or 3.19** Get it at <https://cmake.org/> 
 
 - **[Recommended] Ninja 1.8.2+** Get it at <https://ninja-build.org/> 
 
    Using Ninja is strongly recommended on all platforms to speed up compilation, if you do not wish to use it, make sure to remove the `-GNinja` options from the configuration command.

### Windows Instructions <a name="windows-instructions"></a>

#### Windows specific dependencies <a name="windows-dependencies"></a>

 - **Microsoft Visual Studio 14 (2015) Express** Get it at <http://go.microsoft.com/fwlink/?LinkId=615464>
    
    You may need to use the `/layout` option to generate an offline installer : <https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2015/install/create-an-offline-installation-of-visual-studio?view=vs-2015>
   
    Alternatively, link to an ISO image: <https://go.microsoft.com/fwlink/?LinkId=615448>

 - **Qt 5.12.9** You can download the installer here: <https://download.qt.io/official_releases/qt/5.12/5.12.9/>
 
    To alleviate the need for registration, disconnect your internet connection before starting the installer.

 - **[only for packaging]** **NSIS version 3.04 or higher** Get it at <https://nsis.sourceforge.io/Download>

#### Windows Guidelines <a name="windows-guidelines"></a>

 - **Must Use MSVC build environment Command Prompt (installed with MSVC)**

    It is located at `Windows Start Menu > Visual Studio 2015 > "VS2015 x64 Native Tools Command Prompt"`
    
    If you are unfamilliar with Windows Development, using this Prompt is mandatory for building as it sets appropriate build environement in contrast with a regular cmd.exe Prompt

 - **Work Directory, source and build paths must be short and close to the root Drive**

    The path to those directories must be short because Windows has limitations on the maximum length of file paths and commands.
    Those paths will appear numerous times in the build process, it can quickly reach the maximum limit.
     
  - **The source directory must not be inside the LidarView source code directory**.
  
    Prefer an Architecture like so :
    
    | C:\      | work dir   |
    |----------|------------|
    | C:\src   | source dir |
    | C:\build | build dir  |
    
  - **Moving these directories after configuration or compilation, will break all build environnements and require a full rebuild.**

#### Windows build instructions <a name="windows-build-instructions"></a>

 - Note that the configuration command mentions the subdirectory "*Superbuild*" dir inside the source directory and not the source directory itself.
 - Note that the CMake `Qt5_DIR` path argument must use **forward slashes** on all platforms (Unix PATH format), MSVC would otherwise take `\\Q` as a build option.
 - If you changed the default Qt installation path, you will have to adapt the configuration command.
 - Building from scratch can take from 45 minutes to 2 hours depending on your hardware.
 - By default `-j` will use all cores on your machine, but you can specify the number of cores to use with `-j N`.

    `cd <work-directory>`
    
    `git clone <git url to LidarView-based app repository> --recursive src`
    
    `mkdir <work-directory>\build`
    
    `cd <work-directory>\build`
    
    `cmake <work-directory>\src\Superbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_qt5=True -DQt5_DIR="C:/Qt/Qt5.12.9/5.12.9/msvc2015_64/lib/cmake/Qt5"`
    
    `cmake --build . -j`
    
    `install\bin\<LidarView-based app name>`

---

### Linux build Instructions <a name="linux-instructions"></a>

#### Linux specific dependencies <a name="linux-dependencies"></a>

**Packages**
 - The packages from the following one-liner are needed to build on Ubuntu 18.04 and higher:

    `sudo apt-get install build-essential flex byacc python3.7-dev libxext-dev libxt-dev libbz2-dev zlib1g-dev freeglut3-dev pkg-config libffi-dev libnl-genl-3-dev libprotobuf-dev protobuf-compiler patchelf libopengl0 liblapack3`

    **If using Ubuntu 20.04**

    Add this package: `sudo apt-get install libfreetype-dev` (will change in the future)

**Qt5:**

 - If you system's package manager offers Qt5 with version 5.12.9 or higher (e.g Ubuntu20.04) use:

    `qtbase5-dev qtmultimedia5-dev qttools5-dev qtbase5-private-dev libqt5x11extras5-dev libqt5svg5-dev`

 - On other systems, we recommend you use offline installers from:

    <https://download.qt.io/official_releases/qt/5.12/5.12.9/>
    
**Python3.7.10**

 - If you system's package manager offers Python3 with version 3.7 (e.g Ubuntu18.04) use:

    `sudo apt-get install python3.7-dev`
    
 - On other systems, you can get it from this unofficial repository:

    `sudo add-apt-repository ppa:deadsnakes/ppa && sudo apt-get install python3.7-dev`

 - Or build it from [Official Source](https://www.python.org/downloads/release/python-3710/)

    `sudo apt-get install build-essentials zlib1g-dev`
    
    `./configure --enable-shared && sudo make install`

#### Linux Guidelines <a name="linux-guidelines"></a>

  - **The source directory must not be inside the LidarView source code directory**.
  
    Prefer an Architecture like so :
    
    | /home/username/lidarview       | work dir   |
    |--------------------------------|------------|
    | /home/username/lidarview/src   | source dir |
    | /home/username/lidarview/build | build dir  |
    
  - **Moving these directories after configuration or compilation, will break all build environnements and require a full rebuild.**

#### Linux compiling instructions <a name="linux-build-instructions"></a>

 - Note that the configuration command mentions the subdirectory "*Superbuild*" dir inside the source directory and not the source directory itself.
 - If you changed the default Qt installation path, you will have to adapt the configuration command.
 - Building from scratch can take from 45 minutes to 2 hours depending on your hardware.
 - By default `-j` will use all cores on your machine, but you can specify the number of cores to use with `-j N`.
 
    `cd <work-directory>`

    `git clone <git url to LidarView-based app repository> --recursive src`

    `mkdir <work-directory>/build`

    `cd <work-directory>/build`

     - If you installed Qt5 through your package manager, simply use:
     
    `cmake <work-directory>/src/Superbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_qt5=True`
    
     - Otherwise, also specify this path from the offline installer:
    
    `cmake <work-directory>/src/Superbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_qt5=True -DQt5_DIR="/opt/Qt/Qt5.12.9/5.12.9/msvc2015_64/lib/cmake/Qt5"`
    
    `cmake --build . -j`
    
    `./install/bin/<LidarView-based app name>`

## Enabling additional LidarView features <a name="enabling-additional-lidarview-features"></a>

**Enable SLAM features**

 - Run or re-run CMake configuration command with these additional parameters:
 
    `-DENABLE_ceres=True -DENABLE_nanoflann=True -DENABLE_pcl=True -DLIDARVIEW_BUILD_SLAM=True`

 - Run or re-run CMake build command.
 
    `cmake --build . -j`

  More info about SLAM is available at [How to SLAM with LidarView](https://gitlab.kitware.com/keu-computervision/slam/-/blob/master/paraview_wrapping/doc/How_to_SLAM_with_LidarView.md).


**Enable OpenCV features**

 - Install OpenCV
 
    Get it at <https://opencv.org/>
    
    On UNIX you can use: `sudo apt-get install libavformat-dev libavdevice-dev libavcodec-dev`

 - Run or re-run CMake configuration command with these additional parameters:
 
    `-DENABLE_opencv=True`

 - Run or re-run CMake build command.
 
    `cmake --build . -j`

## Incremental build instructions <a name="incremental-build"></a>

 - If you modified only LidarView sources, you may want to build incrementally as it is much faster than the full build command:

    `cd <work-directory>\build\common-superbuild\lidarview\build`

    `cmake --build . -j --target install`

## Packaging instructions <a name="packaging"></a>

 - Activate the build of tests through a CMake configuration:

    `cd <work-directory>/build`

    `cmake . -DBUILD_TESTING=True`

 - Build with the new configuration:

    `cmake --build . -j`

 - Package using cpack:

    `ctest`

## Testing instructions <a name="testing"></a>
Detailed Instructions to run LidarView-based app Tests: [LidarView Testing Guide](LidarPlugin/Testing/README.md).

## Troubleshooting / FAQ <a name="faq-instructions"></a>

### Superbuild failure with PCL enabled

Depending on your hardware, when enabling (`-DENABLE_pcl=True`) the superbuild might fail during PCL compilation with an *Internal compiler error* due to intense memory allocation.

To work aroudn this issue you can try:
 - Re-running the build command, as successive incremental builds may eventually succeed.
 - Lowering the number of compilation jobs in the build command using the `-j N` option.
