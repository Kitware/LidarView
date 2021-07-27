# LidarView based App Installation Guide

1. [Overview](#overview)
2. [General Information](#general-information)
3. [Runtime Dependencies](#runtime-dependencies)
    1. [Windows](#windows-instructions)
    2. [Ubuntu20.04](#ubuntu20-instructions)
    3. [Ubuntu18.04](#ubuntu18-instructions)
4. [Additional Instructions](#additional-instructions)
5. [Troubleshooting / FAQ ](#faq-instructions)


## Overview <a name="overview"></a>

This document gather information about the required installation steps to run LidarView-based Apps binary releases,
 built with the latest versions the following repositories: 

 - [LVCore](https://gitlab.kitware.com/LidarView/lidarview-core)
 - [LidarView-superbuild](https://gitlab.kitware.com/LidarView/lidarview-superbuild)
 
This document mirrors some information from the [LidarView Developer Guide](https://gitlab.kitware.com/LidarView/lidarview-core/-/blob/master/Documentation/LidarView_Developer_Guide.md) but from an user point of view.

## General Information <a name="general-information"></a>

Note that current LidarView-baseline software stack is typically based on the following major elements:

 - Paraview 5.6.1 (embedded)
 - VTK 8.2 (embedded)
 - Qt 5.12.9 
 - Python 3.7.10

Aside from Runtime compatibility, also keep in mind the following aspects:

 - Paraview/VTK Plugins versions to use
 - Python modules and scripting
 - ThirdParty Bugs

## Required Runtime dependencies <a name="runtime-dependencies"></a>
The following sections details what dependencies to install prior to running a LidarView-based executable and how to install them.

### Windows x64 <a name="windows-instructions"></a>

**Runtime Dependencies:**

* No dependencies required (Everything is shipped in the bundle)

### Ubuntu 20.04 <a name="ubuntu20-instructions"></a>

**Runtime Dependencies:**

* Qt5.12.9 - Available as packages:

    `sudo apt-get install qt5-default libqt5help5 libqt5x11extras5`
        
    (If you do not wish to install those packages, you can use the offline installer locally, see [Additional Instructions](#qt-installer))

        
* Python 3.7.10 - Not available as packages, you can get it from the 'deadsnakes' ppa:

    `sudo add-apt-repository ppa:deadsnakes/ppa`
    
    `sudo apt-get install python3.7-dev`
    
    (If you wish to build Python3.7.10 from the official source instead, see [Additional Instructions](#python-source))
  
* Required packages:

    `sudo apt-get install libopengl0 liblapack3`

### Ubuntu 18.04 <a name="ubuntu18-instructions"></a>

**Runtime Dependencies:**

* QT5.12.9 - Not available as packages - From Installer

    See instructions in section [Additional Instructions](#qt-installer)

* Python 3.7.10 - Available as package:
    
    `sudo apt-get install python3.7-dev`
    
    (If you wish to build Python3.7.10 from the official source instead, see [Additional Instructions](#python-source))
  
* Required packages:

    `sudo apt-get install libopengl0 liblapack3`
    
### Additional Instructions <a name="additional-instructions"></a>

* **QT5.12.9 From Installer** <a name="qt-installer"></a>

    Qt5.12.9 - [Offline Installer](https://download.qt.io/official_releases/qt/5.12/5.12.9/qt-opensource-linux-x64-5.12.9.run)

    Run the installer offline to alleviate the need to register
    
    * Note that only the `Desktop gcc 64-bit` component is needed
    
    Copy built libraries located in `~/Qt5.12.9/5.12.9/gcc_64/lib/` in your system.
    
    * We recommend copying those libs towards `/usr/local/lib`, and adding this directory to your ld configuration using:
    
    * `sudo echo "/usr/local/lib" >> /etc/ld.so.conf && sudo ldconfig`
    
* **Python3.7.10 build from source** <a name="python-source"></a>

    Download [Sources](https://www.python.org/downloads/release/python-3710/)

    Packages `build-essentials, zlib1g-dev` are required to build python

    Build and install with this oneliner:
    
    `./configure --enable-shared && sudo make install`
        
## Troubleshooting / FAQ <a name="faq-instructions"></a>

* **WINDOWS - Unrecognized Publisher**

    Click `More Info -> Run Anyway`

* **WINDOWS - Graphic Bug with interleaved horizontal greenlines**

    Simillar problem to [this](https://discourse.slicer.org/t/green-horizontal-lines-appear-in-slicer-4-10-2-at-startup/12090).
    This occurs on Windows with NVIDIA Optimus mobile (Laptops) graphic cards.

    - Update you Drivers (Use NVIDIA Geforce Experience)
    - Select `High-performance NVIDIA processor` in NVIDIA control panel / Manage 3d settings / Preferred graphics processor

* **UBUNTU - Cannot find Qt Packages "unable to locate package qt5-default"**

    Qt is [community software](https://packages.ubuntu.com/focal/qt5-default), Uncomment / add the `universe` ubuntu PPA in your `/etc/apt/sources.list`
    
    Example: `deb http://fr.archive.ubuntu.com/ubuntu/ focal universe`

* **UBUNTU - There is no application installed for "shared library" files**

    This is a Nautilus configuration bug on Ubuntu, just launch via terminal using `./LidarView`

* **UBUNTU - QT Offline Installer asks for registration**

    Disconnect your internet connection before starting the installer, registration will not be needed
