# LidarView Developer Guide

1. [LidarView Compilation Overview](#lv-compilation-overview)
    1. [Superbuild Overview](#superbuild-overview)
    2. [LidarView dependencies](#dependencies)
        1. [PCAP library](#pcap-library)
        2. [Boost library](#boost-library)
        3. [Qt library](#qt-library)
        4. [Python](#python)
        5. [Python Qt library](#python-qt-library)
        6. [Paraview and VTK](#paraview-vtk)
2. [Configure and build instructions](#configure-build)
    1. [Windows Instructions](#windows-instructions)
        1. [Windows dependencies](#windows-dependencies)
        2. [Windows build instructions](#windows-build-instructions)
    2. [Linux Instructions](#linux-instructions)
        1. [Linux dependencies](#linux-dependencies)
        2. [Linux build instructions](#linux-build-instructions)
    3. [Apple Instructions](#apple-instructions)
3. [Enabling additional LidarView features](#enabling-additional-lidarview-features)
4. [Packaging instructions](#packaging)
5. [Troubleshooting](#troubleshooting)
    1. [Superbuild failure with PCL enabled](#superbuild-failure-with-pcl-enabled)
    2. [Using superbuild with system-wide Boost install](#using-superbuild-with-system-wide-boost-install)

## LidarView Compilation Overview <a name="lv-compilation-overview"></a>

### Superbuild Overview <a name="superbuild-overview"></a>
LidarView can use a cmake *superbuild* to download and compile third party projects that are dependencies of LidarView.
The superbuild is not mandatory but it is recommended as it eases dependency management and build workflow.
The superbuild will **give you the option to use system installations of third party projects instead of compiling them as a superbuild step**.
Some dependencies, on certain platforms, must be compiled by the superbuild, and for them there is no option to use a system version.

### LidarView dependencies <a name="dependencies"></a>
The LidarView application and libraries have several external library dependencies. As explained in the [Superbuild Overview](#superbuild-overview), **most of the dependencies will be downloaded and compiled automatically** during the build step. See [Configure and build instructions](#configure-build).
Below is a non-comprehensive list of LidarView's main dependencies:

 - **PCAP library** <a name="pcap-library"></a>

The required pcap version is 1.4.
Pcap is required to support saving captured packets to a file, and reading files containing saved packets.
On Mac/Linux, only *libpcap* is required.
On Windows, we use the *winpcap* project which includes *libpcap* but also includes Windows specific drivers. Since the winpcap project only provides Visual Studio project files, which may be out dated, the superbuild does not attempt to compile winpcap. Instead, a git repository containing headers and precompiled *.lib* and *.dll* files is used. The repository url is https://github.com/patmarion/winpcap.

 - **Boost library** <a name="boost-library"></a>

The required boost version is 1.66.
Boost is used for threading and synchronization, network communication and handling of the filesystem.

 - **Qt library** <a name="qt-library"></a>

The required Qt version is 5.12.8
Qt is a desktop widget library that is used to provide user interface elements like windows and menus across the supported platforms Windows, Mac, and Linux.

 - **Python** <a name="python"></a>

The required Python version is 3.7.
LidarView uses libpython to embed a Python interpreter in the LidarView application.
The core LidarView features are implemented in C++ libraries, and the libraries are wrapped for Python using VTK's Python wrapping tools.

 - **PythonQt** <a name="python-qt-library"></a>

PythonQt version is "patch_8" (see Superbuild/version.txt).
PythonQt is used to build Qt applications using Python.
PythonQt has support for wrapping types derived from Qt objects and VTK objects.

 - **Paraview (and VTK)** <a name="paraview-vtk"></a>

The required ParaView version is 5.6.1
The required VTK version is 8.2.
The ParaView repository includes VTK, so the superbuild only needs to checkout and build ParaView in order to satisfy both dependencies.
A specific git commit sha1 is used instead of a specific released version.
The commit sha1 is very similar to the release version but with a few commits from the ParaView master branch cherry-picked onto it.
The PythonQtPlugin is a small plugin that initializes the PythonQt library and makes it available in the ParaView Python console.

## Configure and build instructions <a name="configure-build"></a>

### Windows Instructions <a name="windows-instructions"></a>

#### Windows dependencies <a name="windows-dependencies"></a>
- **CMake version 3.18.2** is confirmed to work, CMake is available at <https://cmake.org/>. Lower versions may not work (especially, avoid versions 3.17.4, 3.18.0 and 3.18.1 which may be incompatible with Paraview and thus LidarView, more info [here](https://gitlab.kitware.com/cmake/cmake/-/merge_requests/5094)), higher versions will work.
- **ninja version 1.8.2** or higher, available at <https://github.com/ninja-build/ninja/releases>. There is no installer for this tool. You must extract the binary *ninja.exe* from *ninja-win.zip* and place it inside a directory that is inside your `%PATH%` environnement variable, such as `C:\Windows`.
- **Microsoft Visual Studio** ***14*** (2015) **Express** ("Desktop"). You can use this link to download the installer: <http://go.microsoft.com/fwlink/?LinkId=615464> This installer is pretty simple (no special options).
- **git**: we recommand using "Git for Windows" available at <https://gitforwindows.org/>.
- **Qt 5.10.1** *(this dependency will be built automatically in the future)*. You can download the installer here: <http://download.qt.io/new_archive/qt/5.10/5.10.1/qt-opensource-windows-x86-5.10.1.exe>. When installing you can keep the suggested installation path. Here is a walkthrough of the installer:  click "Next" > "Skip" > "Next" > keep default install path (advised) and click "Next" > Unfold "Qt" then unfold "Qt 5.10.1" and tick "**MSVC 2015 64-bits**" then click "Next" > "Next" > "Install" > wait for it to install then click "Next" > untick "Launch Qt Creator" and click "Finish".
- [only for packaging] **NSIS version 3.04** is confirmed to work, NSIS is available at <https://nsis.sourceforge.io/Download>.

#### Windows build instructions <a name="windows-build-instructions"></a>

1. Create or go to a work directory where you will save LidarView sources and build directories.

    `cd <work-directory>`

    * **The path to this directory must be short** because Windows has limitations on the maximum length of file paths and commands. Since this directory name will appear lots of times, it can quickly reach the maximum limit. To overcome this issue, we strongly suggest that you use a directory close to the root of a drive, like `C:\` or `C:\LV\`.

2. Clone LidarView's source code repository to a directory of your chosing, for example:

    `git clone <git url to LidarView repository> --recursive LV-src`

    * Moving this directoy in the future will break all build environnements that were using it (you will have to redo steps 6. and 7.).
    * **The path to this directory must be short** because Windows has limitations on the maximum length of file paths and commands. We strongly suggest that you clone the sources close to the root of a drive, like `C:\LV-src`.

3. Create a new directory to store the build.

    `mkdir <work-directory>\LV-build`

    * You can use the Windows file explorer to create this directory.
    * **This directory must not be inside the LidarView source code directory**.
    * **The path to this directory must be short** because Windows has limitations on the maximum length of file paths and commands. We strongly suggest that you use a directory close to the root of a drive, like `C:\LV-build`.

4. Open the appropriate command prompt:

    `Windows Start Menu > Visual Studio 2015 > "VS2015 x86 x64 Cross Tools Command Prompt"`

    * *Tip*: for the next steps it is possible to copy a command and then paste it inside the prompt with `shift+insert` or `right-click`.
    * This command prompt has some environnement variables correctly set to allow building (compiler path, etc).
    * Alternatively it is possible to use a standard *cmd.exe* windows prompt (in which Administrative priviledges should not be enabled for security) by entering the command: `"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86_amd64`.

5. Inside the command prompt, go to the build directory you created in step 3 by entering the command

    `cd /d "<work-directory>\LV-build"`

    * Adapt the path to your own build directory created in step 3.
    * `/d` allows to `cd` to a directory that is not on the same drive as your current path.

6. Inside the command prompt configure the build by entering:

    `cmake <work-directory>\LV-src\Superbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_qt5=True -DQt5_DIR="C:/Qt/Qt5.10.1/5.10.1/msvc2015_64/lib/cmake/Qt5"`

    * Note that this command mentions the subdirectory "*Superbuild*" inside the source directory and not the source directory itself.
    * Note that the `Qt5_DIR` path must use **forward slashes** (like if it was an Unix PATH), because MSVC would otherwise take `\\Q` as a build option.
    * If you changed the default Qt installation path, you will have to adapt this command.
    * You can use absolute or relative path to point to the LidarView source directory, but be sure to adapt its path to the directory created step 2.
    * Should you want to build in RelWithDebInfo mode (in order to attach a debugger for instance), replace "Release" by "RelWithDebInfo".
    * This command should show no errors, else they must be fixed.
    * To enable additional features, see section [Enabling additional LidarView features](#enabling-additional-lidarview-features).

7. Inside the command prompt, start building by entering:

    `ninja`

    * Building from scratch can take from 45 minutes to 3 hours depending on your hardware.
    * By default ninja will use all cores on your machine, but you can restrict the number of cores used by using `ninja -jN` (replace N by the number of cores to use).

8. If you modified only LidarView and want to rebuild incrementally (incrementally = only modified files are rebuilt), enter the commands:

    `cd <work-directory>\LV-build\common-superbuild\lidarview\build`

    `ninja install`

    * Incremental builds are much faster than the first build.
    * It is also possible to run `ninja` from the top of the build directory like you did the first time, but that will take longer because all dependencies are checked for changes.

9. If you checkout a new branch you will need to update all submodules :

    `git checkout <branch>`

    `git submodule update --init --recursive`

---

### Linux build Instructions <a name="linux-instructions"></a>

#### Linux dependencies <a name="linux-dependencies"></a>
The following packages are needed to build on Ubuntu 16.04/18.04/20.04:

- **CMake version 3.12 or higher** is needed. As the `cmake` package available from Ubuntu may not compatible, you need to get a supported version at <https://cmake.org/>.
- build-essential
- git
- flex byacc
- python3.7-dev
- libxext-dev libxt-dev
- libbz2-dev zlib1g-dev
- freeglut3-dev
- pkg-config
- libffi-dev

The Ubuntu 18.04 one-liner is: `sudo apt-get install build-essential git flex byacc python3.7-dev libxext-dev libxt-dev libbz2-dev zlib1g-dev freeglut3-dev pkg-config libffi-dev`


**If using Ubuntu 20.04**

- libnl-gen1-3-dev
- libprotobuf-dev protobuf-compiler
- libfreetype-dev

The Ubuntu 18.04 one-liner is: `sudo apt-get install build-essential git flex byacc python3.7-dev libxext-dev libxt-dev libbz2-dev zlib1g-dev freeglut3-dev pkg-config libffi-dev libnl-gen1-3-dev libprotobuf-dev protobuf-compiler libfreetype-dev`

**If using qt5 system:**

- qtbase5-dev qtmultimedia5-dev qttools5-dev qtbase5-private-dev libqt5x11extras5-dev libqt5svg5-dev

**If opencv if enabled:**

- libavformat-dev libavdevice-dev libavcodec-dev

#### Linux compiling instructions <a name="linux-build-instructions"></a>

1. Clone LidarView's source code repository to a directory of your chosing, for example:

    `cd <work-directory>`

    `git clone <git url to LidarView repository> --recursive LidarView-source`

    Note: Moving this directoy in the future will break all build environnements that were using it (you will have to redo steps 3. and 4.).

2. Create a new directory to store the build.

    `mkdir <work-directory>/LidarView-build`

    * **This directory must not be inside the LidarView-source code directory**

3. Configure the build by entering:

    `cd <work-directory>/LidarView-build`

    `cmake <work-directory>/LidarView-source/Superbuild -DCMAKE_BUILD_TYPE=Release`

    * By default the generator used is **make**, if you prefer to use **ninja**, add the option `-GNinja`.
    * Take note that this command mentions the subdirectory "*Superbuild*" inside the source directory and not the source directory itself.
    * To enable additional features, see section [Enabling additional LidarView features](#enabling-additional-lidarview-features).

4. Start building by entering:

    `make -j<N>`

   * Replace `<N>` by the number of cores you want to use.

5. If you modified only LidarView and want to rebuild incrementally (incrementally = only modified files are rebuilt), enter the commands:

    `cd lidarview-superbuild/common-superbuild/lidarview/build`

    `make install`

    * Incremental builds are much faster than the first build.
    * It is also possible to run `make` from the top of the build directory like you did the first time, but that will take longer because all dependencies are checked for changes.

6. If you checkout a new branch you will need to update all submodules :

    `git checkout <branch>`

    `git submodule update --init --recursive`

---

### Apple Instructions <a name="apple-instructions"></a>

## Enabling additional LidarView features <a name="enabling-additional-lidarview-features"></a>

The command presented in step Windows/6 or Linux/3 enables basic LidarView features, which use basic dependencies.
To enable more features that rely on additional external libraries, you can use the `ENABLE_<lib>=True` flags. This will download and install the required dependencies, and enable the corresponding LidarView features.

For example, to build LidarView with SLAM support, use `-DENABLE_ceres=True -DENABLE_nanoflann=True -DENABLE_pcl=True -DLIDARVIEW_BUILD_SLAM=True` (more info in [How to SLAM with LidarView](https://gitlab.kitware.com/keu-computervision/slam/-/blob/master/paraview_wrapping/doc/How_to_SLAM_with_LidarView.md)).

## Packaging instructions <a name="packaging"></a>

1. Activate the build of tests in the cmake configuration:

    `cd <work-directory>/LidarView-build`

    `cmake . -DBUILD_TESTING=True`

2. Build with the new configuration:

    * On Windows : `ninja`
    * On Linux : `make`
    * Or on all OS : `cmake --build .`

3. Package using cpack:

    `ctest -R cpack`

## Troubleshooting

### Superbuild failure with PCL enabled

It has been reported that if PCL is enabled in the superbuild (`-DENABLE_pcl=True`), the superbuild might fail during PCL compilation with a message such as *Internal compiler error*. This bug has been reported on Linux and Windows.

PCL is a large point cloud processing library. Some binaries are quite heavy, and need to build/link against many targets. Sometimes, depending on your machine, the build process of some of these binaries may reach the maximum allocatable memory limit. When this happens, the OS kills this process, resulting in the *internal compiler error*.

If you run into this problem, you can first try to launch again the superbuild (just re-run the `ninja/make` command, or even easier, use `cmake --build` that will call the right generator for you.
If this fails again, try to build the PCL project with less jobs, then resume the superbuild :

```bash
cd <LidarView-build>/lidarview-superbuild/common-superbuild/pcl/build/
cmake --build . --target --install -- -j1   # build and install PCL project with only 1 job
cd <LidarView-build>
cmake --build .  # resume whole superbuild
```

### Using superbuild with system-wide Boost install

Boost libraries can be tricky to install and to manage, especially as they do not rely on CMake, and if different versions are available on your machine. To avoid these problems, the Superbuild will by default install all the Boost components it needs, and will build LidarView against this local Boost installation.

However, if you have a system-wide installation of Boost on your machine, and especially if you defined some global environment variables related to Boost, the local installation may be hidden or ignored by CMake. Therefore, if your global install lacks some of the required components, or if there is any ABI incompatibility between the global and local libraries, compilation or runtime errors may arise.

To avoid these issues, delete your current superbuild build directory, then to rebuild it, either :
- **Use the superbuild Boost (preferred)** : remove the Boost environment variables during the compilation process (`BOOST_ROOT`, `BOOSTROOT`, `BOOST_LIBARYDIR`, `BOOST_INCLUDEDIR`, ...).
- **Use only your system Boost** : tell the superbuild to use your system-wide install instead of installing a local one by enabling flag `USE_SYSTEM_boost=True` in the CMake superbuild configuration.
