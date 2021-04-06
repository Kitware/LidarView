# LidarView Releases

## LidarView v1.4.1

### Ubuntu 20.04 ([Download Link](http://www.example.org))

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

### Ubuntu 18 ([Download Link](http://www.example.org))

**Runtime Dependencies:**



### Windows x64 ([Download Link](http://www.example.org))

**Runtime Dependencies:**

* No dependencies required

### MacOS ([Download Link](http://www.example.org))

**Runtime Dependencies:**

* Qt5.12.8 - [Installer](https://download.qt.io/official_releases/qt/5.12/5.12.8/qt-opensource-linux-x64-5.12.8.run)

* Python 3.7.10 - [Sources](https://www.python.org/downloads/release/python-3710/)
