#!/bin/sh

set -e

readonly nanoflann_repo="https://github.com/jlblancoc/nanoflann.git"
readonly nanoflann_commit="v1.4.3"

readonly nanoflann_root="$HOME/nanoflann"
readonly nanoflann_src="$nanoflann_root/src"
readonly nanoflann_build_root="$nanoflann_root/build"

git clone "$nanoflann_repo" "$nanoflann_src"
git -C "$nanoflann_src" checkout "$nanoflann_commit"

nanoflann_build () {
    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$nanoflann_src" \
        -B "$nanoflann_build_root" \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$nanoflann_build_root" --target install
}

nanoflann_build /usr

rm -rf "$nanoflann_root"
