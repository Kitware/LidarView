# LidarView Developer Guide

1. [Overview](#lv-compilation-overview)
2. [Configure & Build instructions](#congfigure-build)
    1. [Crossplatform](#crossplatform-instructions)
    2. [Windows](#windows-instructions)
    3. [Ubuntu](#linux-instructions)
3. [Enabling Additional Features](#enabling-additional-lidarview-features)
4. [Incremental Build Instructions](#incremental-build)
5. [Debug Build Instructions](#debug-build)
6. [Packaging Instructions](#packaging)
7. [Testing Instructions](#testing)
8. [Additional Intructions](#additional-instructions)
9. [Troubleshooting / FAQ ](#faq-instructions)

## LidarView Compilation Overview <a name="lv-compilation-overview"></a>

This documents gather all information needed to build LidarView-based Applications.

This document mirrors some information from the [Installation Guide](Documentation/INSTALLATION.md).

LidarView relies primarily on a CMake and the *superbuild* system, it handles the download and compilation of most, if not all, necessary dependencies.

However some dependencies, may or may not be supplied by the superbuild for various reasons and must be obtained externally on some platforms,
 see the following sections for per platform instructions.

LidarView is actively Maintained on Windows, Ubuntu18.04, Ubuntu20.04 and OSX.

## Configure and build instructions <a name="configure-build"></a>

### Crossplatform Instructions <a name="crossplatform-instructions"></a>

**Crossplatform Dependencies and Tools**

The specific version of the following tools may or may not be available in your OS package manager.

 - **[Required] CMake 3.20.3+** Get it at <https://cmake.org/>

 - **[Recommended] Ninja 1.8.2+** Get it at <https://ninja-build.org/>

    Using Ninja is strongly recommended on all platforms to speed up compilation, if you do not wish to use it, make sure to remove the `-GNinja` options from the configuration command.

### Windows Instructions <a name="windows-instructions"></a>

#### Windows specific dependencies <a name="windows-dependencies"></a>

 - **Microsoft Visual Studio 2019** Only the 2019 MSVC version is supported.

 - **Qt 5.12.9** You must get it through the offline installer (Building Qt5 from source is a lengthy process)

    For more details, see [Additional Instructions](#qt-installer).

 - **[only for packaging]** **NSIS version 3.04 or higher** Get it at <https://nsis.sourceforge.io/Download>

#### Windows Guidelines <a name="windows-guidelines"></a>

 - **Must Use MSVC build environment Command Prompt (installed with MSVC)**

    It is located at `Windows Start Menu > Visual Studio 20XX > "VS20XX x64 Native Tools Command Prompt"`

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

    **Moving these directories after configuration or compilation, will break all build environnements and require a full rebuild.**

#### Windows build instructions <a name="windows-build-instructions"></a>

 - Note that the configuration command mentions the subdirectory "*Superbuild*" dir inside the source directory and not the source directory itself.
 - Note that CMake variables, like `Qt5_DIR` path argument,  must use **forward slashes** on all platforms (Unix PATH format), MSVC would otherwise take `\\Q` as a build option.
 - If you changed the default Qt installation path, you will have to adapt the configuration command.
 - Building from scratch can take anywhere from 20 minutes to 2 hours depending on your hardware.
 - By default `-j` will use all cores on your machine, but you can specify the number of cores to use with `-j N`.

    `cd <work-directory>`

    `git clone <git url to LidarView-based app repository> --recurse-submodules src`

    `mkdir <work-directory>\build`

    `cd <work-directory>\build`

    `cmake <work-directory>\src\Superbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_qt5=True -DQt5_DIR="C:/Qt/Qt5.12.9/5.12.9/msvc2017_64/lib/cmake/Qt5"`

    `cmake --build . -j`

    `install\bin\<LidarView-based app name>`

---

### Linux build Instructions <a name="linux-instructions"></a>

#### Linux specific dependencies <a name="linux-dependencies"></a>


**Graphics Drivers**
 - Make sure graphics drivers are up-to date, ensuring essential graphics packages are installed
 
    If you do not have a graphics card, `mesa` drivers will need to be installed.

**Packages**
 - The packages from the following one-liner are needed to build on Ubuntu 18.04 and higher:

    `sudo apt-get install build-essential byacc flex freeglut3-dev libbz2-dev libffi-dev libfontconfig1-dev libfreetype6-dev libnl-genl-3-dev libopengl0 libprotobuf-dev libx11-dev libx11-xcb-dev libx11-xcb-dev libxcb-glx0-dev libxcb-glx0-dev libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-shape0-dev libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-xkb-dev libxcb1-dev libxext-dev libxext-dev libxfixes-dev libxi-dev libxkbcommon-dev libxkbcommon-dev libxkbcommon-x11-dev libxkbcommon-x11-dev libxrender-dev libxt-dev pkg-config protobuf-compiler zlib1g-dev`

 - Additionally For Ubuntu20: `sudo apt-get install libglx-dev`

**[OPTIONAL] Qt5:**

  Qt5 is built automatically by the Superbuild, however to speed up the build process, you can opt to use built-binaries with the following options:

 - If you system's package manager offers Qt5 with version 5.12.9 or higher (e.g Ubuntu20.04) use:

    `qt5-default qtmultimedia5-dev qtbase5-private-dev libqt5x11extras5-dev libqt5svg5-dev qttools5-dev`

    Add this parameter to CMake configuration options: `-DUSE_SYSTEM_qt5=ON`

 - Using an offline installers:

    For more details, see: [Additional Instructions](#qt-installer)

#### Linux Guidelines <a name="linux-guidelines"></a>

  - **The source directory must not be inside the LidarView source code directory**.

    Prefer an Architecture like so :

    | /home/username/lidarview       | work dir   |
    |--------------------------------|------------|
    | /home/username/lidarview/src   | source dir |
    | /home/username/lidarview/build | build dir  |

    **Moving these directories after configuration or compilation, will break all build environnements and require a full rebuild.**

#### Linux compiling instructions <a name="linux-build-instructions"></a>

 - Note that the configuration command mentions the subdirectory "*Superbuild*" dir inside the source directory and not the source directory itself.
 - If you changed the default Qt installation path, you will have to adapt the configuration command.
 - Building from scratch can take from 20 minutes to 2 hours depending on your hardware.
 - By default `-j` will use all cores on your machine, but you can specify the number of cores to use with `-j N`.
 - Do not forget Qt5 configuration options if you opted to use pre-built binaries instead of the default Superbuild compilation.

    `cd <work-directory>`

    `git clone <git url to LidarView-based app repository> --recurse-submodules src`

    `mkdir <work-directory>/build`

    `cd <work-directory>/build`

    `cmake <work-directory>/src/Superbuild -GNinja -DCMAKE_BUILD_TYPE=Release`

    `cmake --build . -j`

    `./install/bin/<LidarView-based app name>`

## Enabling additional LidarView features <a name="enabling-additional-lidarview-features"></a>

**Enable SLAM features**

 - On **UNIX** additional package is needed: `sudo apt-get install liblapack-dev`

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


## Debug build instructions <a name="debug-build"></a>

  - The complete superbuild cannot be reliably built in debug mode, however individual projects, like LidarView itself can be build in `Debug` or `RelWithDebInfo` mode:

    `cd <work-directory>\build\common-superbuild\lidarview\build`

    `cmake . -DCMAKE_BUILD_TYPE=Debug`

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
Detailed Instructions to run LidarView-based app Tests: [LidarView Testing Guide](LidarPlugin/Plugin/LidarCore/Testing/README.md).

## Additional instructions <a name="additional-instructions"></a>

 - **Get QT5.12.9 From Installer** <a name="qt-installer"></a>

    Qt5.12.9 - [Offline Installer](https://download.qt.io/official_releases/qt/5.12/5.12.9/)

    Run the installer offline to alleviate the need to register

    * Note that only the `Desktop 64-bit` component is needed

    * Installation location:

      - On Unix we recommend a `/opt` installation, and adding this directory to your ld configuration using:

        `sudo echo "/usr/local/lib" >> /etc/ld.so.conf && sudo ldconfig`

      - On Windows we recommend a `C:\` installation


    * Add proper parameters to CMake configuration options: `-DUSE_SYSTEM_qt5=ON -DQt5_DIR="/path/to/install/location/lib/cmake/Qt5"`

      **Always forward slashes, UNIX style, on all platforms**

      e.g If installed in `/opt`: `-DQt5_DIR=/opt/Qt5.12.9/5.12.9/gcc_64/lib/cmake/Qt5`

      e.g If installed in `C:\` : `-DQt5_DIR=C:/Qt/Qt5.12.9/5.12.9/msvc2017_64/lib/cmake/Qt5`


## Troubleshooting / FAQ <a name="faq-instructions"></a>

### **UBUNTU** Cannot find Qt Packages "unable to locate package qt5-default"

    Qt is [community software](https://packages.ubuntu.com/focal/qt5-default), Uncomment / add the `universe` ubuntu PPA in your `/etc/apt/sources.list`

    Example: `deb http://archive.ubuntu.com/ubuntu/ focal universe`

### Superbuild failure with PCL enabled

Depending on your hardware, when enabling (`-DENABLE_pcl=True`) the superbuild might fail during PCL compilation with an *Internal compiler error* due to intense memory allocation.

To work aroudn this issue you can try:
 - Re-running the build command, as successive incremental builds may eventually succeed.
 - Lowering the number of compilation jobs in the build command using the `-j N` option.
