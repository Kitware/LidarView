#!/bin/sh

set -e

readonly version="2.1.0"
readonly tarball="ceres-solver-$version.tar.gz"
readonly sha256sum="f7d74eecde0aed75bfc51ec48c91d01fe16a6bf16bce1987a7073286701e2fc6"

readonly ceres_root="$HOME/ceres"
readonly ceres_src="$ceres_root/src"
readonly ceres_build_root="$ceres_root/build"

mkdir -p "$ceres_root"
cd "$ceres_root"

echo "$sha256sum  $tarball" > ceres.sha256sum
curl -OL "http://ceres-solver.org/ceres-solver-$version.tar.gz"
sha256sum --check ceres.sha256sum
mkdir -p "$ceres_src"
tar -xf "$tarball" -C "$ceres_src" --strip-components 1

ceres_build () {
    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$ceres_src" \
        -B "$ceres_build_root" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=OFF \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$ceres_build_root" --target install
}

ceres_build /usr

rm -rf "$ceres_root"
