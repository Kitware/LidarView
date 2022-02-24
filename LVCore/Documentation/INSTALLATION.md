# LidarView based App Installation Guide

1. [Overview](#overview)
2. [General Information](#general-information)
3. [Runtime Dependencies](#runtime-dependencies)
    1. [Windows](#windows-instructions)
    2. [Ubuntu20.04](#ubuntu20-instructions)
    3. [Ubuntu18.04](#ubuntu18-instructions)
    4. [SLAM](#slam-instructions)
4. [Troubleshooting / FAQ ](#faq-instructions)


## Overview <a name="overview"></a>

This document gather information about the required installation steps to run LidarView-based Apps binary releases,
 built with the latest versions the following repositories:

 - [LVCore](https://gitlab.kitware.com/LidarView/lidarview-core)
 - [LidarView-superbuild](https://gitlab.kitware.com/LidarView/lidarview-superbuild)

This document mirrors some information from the [LidarView Developer Guide](https://gitlab.kitware.com/LidarView/lidarview-core/-/blob/master/Documentation/LidarView_Developer_Guide.md) but from an user point of view.

## General Information <a name="general-information"></a>

Note that current LidarView-baseline software stack is typically based on the following embedded elements:

 - Paraview 5.10.0
 - VTK 9.0.2
 - Qt 5.12.9
 - Python 3.9.5

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

* Required packages: `sudo apt-get install libopengl0`

### Ubuntu 18.04 <a name="ubuntu18-instructions"></a>

**Runtime Dependencies:**

* Required packages: `sudo apt-get install libopengl0`

### SLAM <a name="slam-instructions"></a>

**Runtime Dependencies:**

If your LidarView-based binary was supplied with SLAM, install LAPACK package on Ubuntu20.04/18.04

* Required packages: `sudo apt-get install liblapack3`

## Troubleshooting / FAQ <a name="faq-instructions"></a>

* **WINDOWS - Unrecognized Publisher**

    Click `More Info -> Run Anyway`

* **WINDOWS - Graphic Bug with interleaved horizontal greenlines**

    Simillar problem to [this](https://discourse.slicer.org/t/green-horizontal-lines-appear-in-slicer-4-10-2-at-startup/12090).
    This occurs on Windows with NVIDIA Optimus mobile (Laptops) graphic cards.

    - Update you Drivers (Use NVIDIA Geforce Experience)
    - Select `High-performance NVIDIA processor` in NVIDIA control panel / Manage 3d settings / Preferred graphics processor

* **UBUNTU - There is no application installed for "shared library" files**

    This is a Nautilus configuration bug on Ubuntu, just launch via terminal using `./LidarView`

* **UBUNTU - QT Offline Installer asks for registration**

    Disconnect your internet connection before starting the installer, registration will not be needed
