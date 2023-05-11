#!/bin/sh

set -e

readonly paraview_repo="https://gitlab.kitware.com/paraview/paraview.git"
readonly paraview_commit="b5bd6d8beff4d9acdce5b44e0da951710809e4ca" # Waiting for new release

readonly paraview_root="$HOME/paraview"
readonly paraview_src="$paraview_root/src"
readonly paraview_build_root="$paraview_root/build"

git clone "$paraview_repo" "$paraview_src" --recursive
git -C "$paraview_src" checkout "$paraview_commit"

paraview_build () {
    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$paraview_src" \
        -B "$paraview_build_root" \
        -DCMAKE_BUILD_TYPE=Release \
        -DPARAVIEW_BUILD_TESTING=OFF \
        -DPARAVIEW_ENABLE_GDAL=ON \
        -DPARAVIEW_ENABLE_PDAL=ON \
        -DPARAVIEW_ENABLE_LAS=ON \
        -DPARAVIEW_USE_VTKM=OFF \
        -DPARAVIEW_USE_QT=ON \
        -DPARAVIEW_USE_PYTHON=ON \
        -DPARAVIEW_PLUGIN_ENABLE_PythonQtPlugin=ON \
        -DPARAVIEW_PLUGIN_AUTOLOAD_PythonQtPlugin=ON \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$paraview_build_root" --target install
}

paraview_build /usr

rm -rf "$paraview_root"
