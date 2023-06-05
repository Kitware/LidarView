#!/bin/sh

set -e

readonly liblas_repo="https://github.com/libLAS/libLAS.git"
readonly liblas_commit="1.8.1"

readonly liblas_root="$HOME/liblas"
readonly liblas_src="$liblas_root/src"
readonly liblas_build_root="$liblas_root/build"

git clone "$liblas_repo" "$liblas_src"
git -C "$liblas_src" checkout "$liblas_commit"
git -C "$liblas_src" apply /root/patches/las-new-boost.patch

liblas_build () {
    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$liblas_src" \
        -B "$liblas_build_root" \
        -DWITH_GDAL:BOOL=FALSE \
        -DBUILD_OSGEO4W:BOOL=OFF \
        -DWITH_GEOTIFF:BOOL=FALSE \
        -DWITH_LASZIP:BOOL=FALSE \
        -DWITH_TESTS:BOOL=FALSE \
        -DWITH_UTILITIES:BOOL=FALSE \
        -DBoost_USE_STATIC_LIBS:BOOL=FALSE \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$liblas_build_root" --target install
}

liblas_build /usr

rm -rf "$liblas_root"
