## ci-windows

- `lv` -> LidarView ci doesn't support UI tests on all platforms.
- `Python` -> Segfault need investigation.
- `TestVelodyneLidarStream_HDL_64` -> Stream frame delayed since [velodyneplugin!12](https://gitlab.kitware.com/LidarView/velodyneplugin/-/merge_requests/12) update


## ci-ubuntu22-debug

- `TestVelodyneHDLPositionReader` -> doesn't find proj.db, gdal issue?
