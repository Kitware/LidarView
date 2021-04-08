# LidarView Releases

See Download Links in the [Release](https://gitlab.kitware.com/LidarView/lidarview/-/releases) page of this repository.

## LidarView v4.1.5

1. [Windows](#windows-instructions)
2. [Ubuntu20.04](#ubuntu20-instructions)
3. [Ubuntu18.04](#ubuntu18-instructions)
4. [Additional Instructions](#additional-instructions)
5. [Troubleshooting / FAQ ](#faq-instructions)

### Windows x64 <a name="windows-instructions"></a>

**Runtime Dependencies:**

* No dependencies required

### Ubuntu 20.04 <a name="ubuntu20-instructions"></a>

**Runtime Dependencies:**

* Qt5.12.8 - Packages available on ubuntu:

    `sudo apt-get install qt5-default libqt5help5 libqt5x11extras5`
        
    (If you do not wish to install those packages, you can use the offline installer, see [Additional Instructions](#qt-installer))

        
* Python 3.7.10, not available as packages, you can get it from the 'deadsnakes' ppa:
    
    `sudo add-apt-repository ppa:deadsnakes/ppa`
    
    `sudo apt-get install python3.7-dev`
    
    (If you wish to build Python3.7.10 from the official source instead, see [Additional Instructions](#python-source))
  
* Required packages:
    `sudo apt-get install libopengl0`

### Ubuntu 18.04 <a name="ubuntu18-instructions"></a>

**Runtime Dependencies:**

* Python 3.7.10, available as packages `sudo apt-get install python3.7-dev`
  
* Required packages:
    `sudo apt-get install libopengl0`

(Note: this package is forward compatible with Ubuntu 20.04 with minor fixes, see [Additional Instructions](#ubuntu-forward))

### Additional Instructions <a name="additional-instructions"></a>

* **QT5.12.8 From Installer** <a name="qt-installer"></a>

    Qt5.12.8 - [Offline Installer](https://download.qt.io/official_releases/qt/5.12/5.12.8/qt-opensource-linux-x64-5.12.8.run)

    Run the installer offline to alleviate the need to register
    
    * Note that only the `Desktop gcc 64-bit` component is needed
    
    Copy built libraries located in `~/Qt5.12.8/5.12.8/gcc_64/lib/` in your system.
    
    * We recommend copying those libs towards `/usr/local/lib`, and adding this directory to your ld configuration using:
    
    * `sudo echo "/usr/local/lib" >> /etc/ld.so.conf && sudo ldconfig`
    
* **Python3.7.10 build from source** <a name="python-source"></a>

    Download [Sources](https://www.python.org/downloads/release/python-3710/)

    Packages `build-essentials, zlib1g-dev` are required to build python

    Build and install with this oneliner:
    
    `./configure --enable-shared && sudo make install`
        
* **Using Ubuntu18.04 package on Ubuntu20.04** <a name="ubuntu-forward"></a>
    
    Required Packages:

    `sudo apt-get install python3.7-dev`

    `sudo apt-get install liblapack3`

    `sudo apt-get install libopengl0`
    
    Minor libdouble-conversion version compatibility fix:
    
    `sudo ln -s /usr/lib/x86_64-linux-gnu/libdouble-conversion.so.3 /usr/lib/x86_64-linux-gnu/libdouble-conversion.so.1`

### Troubleshooting / FAQ <a name="faq-instructions"></a>

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
