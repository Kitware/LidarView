# LidarView Testing Guide
## Tests Definitions
### Reader tests definition
Reader tests are based on the comparison of the processing of a reference recording
**pcap** file with a **vtp** baseline file, that has been generated on a **stable version** of the Reader.

## Run Tests
### Get Test data and baselines
Test Data and baselines are stored within **Git Large File Storage (LFS)**.

If `git-lfs` was already installed prior to cloning LidarView's main repository and its submodules, Test Data should already have been downloaded.

If this is not the case, or you wish to update LFS files to the origin's versions use :
```
git lfs fetch --all && git lfs pull && \
git submodule foreach --recursive "git lfs fetch --all && git lfs pull"
```

TIP: If you want an overview of every LFS files use:
```
echo 'LidarView' && \
git lfs ls-files --all | sed 's/^/    /' && \
git submodule foreach --recursive "git lfs ls-files --all | sed 's/^/    /' "
```

TIP: If you wish to clone without Test Data use:
```
GIT_LFS_SKIP_SMUDGE=1 git clone ...
```

## Enable Testing and Run the Tests
### Enable Testing
In LidarView's build directory, enable CMAKE option `BUILD_TESTING`, then rebuild LidarView:
```
cmake . -DBUILD_TESTING=ON
cmake --build . --target install
```

### Run the tests
From the LidarView build directory, use CTest:
```
ctest -N
ctest -R <REGEX_TEST_NAME> [-VV]
```

**Using CTest on Windows:** On Windows, you need to add the option
`-C <debug/release>` according to your build type.

### Generate Baselines
**Disclaimer:** In the event that a significant change in how LidarView should process and ouput Data,
the baselines would become irrelevant and need to be generated again for the Tests to pass again.

To generate new baselines automatically, invoke LidarView with the `--script`
option using the `generateTestData.py` script.

On Linux:
```
INSTALL_DIR/bin/LidarView --script=BUILD_DIR/lidarview/src/lidarview-build/bin/generateTestData.py
```
On Windows:
```
INSTALL_DIR/bin/LidarView.exe --script=INSTALL_DIR/bin/generateTestData.py
```
On MacOS:
```
open PACKAGE_DIR/LidarView.app --args --script=BUILD_DIR/lidarview/src/lidarview-build/bin/generateTestData.py
```
Updated test data will be generated in `/TestData`. Commit your changes in the
submodule `TestData` first and then commit them on this repository.


### Adding new test data
Adding new test data means adding a PCAP file and associated VTP baseline files to
LidarView-TestData. It has to be saved under `TestData`. If you need a custom
calibration file, it has to be in the `share` directory. Then, edit
`generateTestData.py.in` in order to add your PCAP and its associated calibration
file to the list of tests data to generate.

Finaly re generate LidarView with cmake, in order to create a new 'generateTestData.py' file
and add a new test in the `CMakeList.txt`.

Don't forget to commit your changes!


**Requirement for HDL-64 live calibration**: 
The live calibration mode computes the calibration on the fly from latest received data packet.
Therefore your PCAP file needs to have a minimum of 12480 packets for it be computed correctly.  

Note: The rolling calibration data span 4160 datapacket, but 3x redundancy is required to eliminate side-effects.

