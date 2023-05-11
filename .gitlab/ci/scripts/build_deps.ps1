$lvsb_path = "C:\lidarview-ci\LVSB-DEPS"
$qtbinaries = "C:\Qt\Qt5.12.9\5.12.9\msvc2017_64\bin"

cmake -DLVSB_PATH=$lvsb_path -P .gitlab/ci/download_superbuild_deps.cmake
cmake -S $lvsb_path/src -B $lvsb_path -C .gitlab/ci/configure_superbuild_deps.cmake -GNinja -DQTBINARIES=$qtbinaries
cmake --build $lvsb_path --parallel 8
