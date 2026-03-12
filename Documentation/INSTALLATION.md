# LidarView based App Installation Guide

1. [Overview](#overview)
2. [General Information](#general-information)
3. [Runtime Dependencies](#runtime-dependencies)
    1. [Windows](#windows-instructions)
    2. [Ubuntu24.04](#ubuntu24-instructions)
4. [Troubleshooting / FAQ ](#faq-instructions)

## Overview <a name="overview"></a>

This document gathers information about the required installation steps to run LidarView-based apps.

## General Information <a name="general-information"></a>

Note that current LidarView-baseline software stack is typically based on the following embedded elements:

 - Latest [ParaView](https://gitlab.kitware.com/paraview/paraview/-/tags) major version
 - VTK integrated from ParaView
 - Qt 6
 - Python 3.12

Aside from Runtime compatibility, also keep in mind the following aspects:

 - Paraview/VTK Plugins versions to use
 - Python modules and scripting
 - ThirdParty Bugs

## Required Runtime dependencies <a name="runtime-dependencies"></a>

The following sections details what dependencies to install prior to running a LidarView-based executable and how to install them.

### Windows x64 <a name="windows-instructions"></a>

**Runtime Dependencies:**

* No dependencies required (Everything is shipped in the bundle)

### Ubuntu 24.04 <a name="ubuntu22-instructions"></a>

**Runtime Dependencies:**

* Required packages: `sudo apt-get install libgomp1 libxcb-cursor0 libxcb-xinerama0 libxcb-xinput0 libquadmath0`

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
