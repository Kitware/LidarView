# LidarView Releases

See Download Links in the [Release](https://gitlab.kitware.com/LidarView/lidarview/-/releases) page of this repository.

## LidarView v1.4.1

### Ubuntu 20.04

**Runtime Dependencies:**

* Qt5.12.8 - [Installer](https://download.qt.io/official_releases/qt/5.12/5.12.8/qt-opensource-linux-x64-5.12.8.run)

    Run the installer offline to alleviate the need to register
    
    * Note that only the `Desktop gcc 64-bit` component is needed
    
    Copy built libraries located in `~/Qt5.12.8/5.12.8/gcc_64/lib/` in your system.
    
    * We recommend copying those libs towards `/usr/local/lib`, and adding this directory to your ld configuration using:
    
    * `sudo echo "/usr/local/lib" >> /etc/ld.so.conf && sudo ldconfig`
    
* Python 3.7.10 - [Sources](https://www.python.org/downloads/release/python-3710/)

    Packages `build-essentials, zlib1g-dev` are required to build python

    Build and install with this oneliner:
    
    `./configure --enable-shared && sudo make install`
  
* Required packages: `libopengl0`

### Ubuntu 18

**Runtime Dependencies:**

### Windows x64

**Runtime Dependencies:**

* No dependencies required

### MacOS

**Runtime Dependencies:**

* Qt5.12.8 - [Installer](https://download.qt.io/official_releases/qt/5.12/5.12.8/qt-opensource-linux-x64-5.12.8.run)

* Python 3.7.10 - [Sources](https://www.python.org/downloads/release/python-3710/)

### Troubleshooting 

**Graphic Bug with interleaved horizontal greenlines**
Simillar problem to [this](https://discourse.slicer.org/t/green-horizontal-lines-appear-in-slicer-4-10-2-at-startup/12090).
This occurs on Windows with NVIDIA Optimus mobile (Laptops) graphic cards.

- Update you Drivers (Use NVIDIA Geforce Experience)
- Select `High-performance NVIDIA processor` in NVIDIA control panel / Manage 3d settings / Preferred graphics processor

**There is no application installed for "shared library" files**
This is a Nautilus configuration bug on Ubuntu, just launch via terminal using `./LidarView`
