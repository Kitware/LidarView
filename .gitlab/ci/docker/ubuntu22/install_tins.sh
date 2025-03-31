#!/bin/sh

set -e

readonly version="4.5"
readonly tarball="v$version.tar.gz"
readonly sha256sum="6ff5fe1ada10daef8538743dccb9c9b3e19d05d028ffdc24838e62ff3fc55841"

readonly tins_root="$HOME/tins"
readonly tins_src="$tins_root/src"
readonly tins_build_root="$tins_root/build"

mkdir -p "$tins_root"
cd "$tins_root"

echo "$sha256sum  $tarball" > tins.sha256sum
curl -OL "https://github.com/mfontanini/libtins/archive/refs/tags/v$version.tar.gz"
sha256sum --check tins.sha256sum
mkdir -p "$tins_src"
tar -xf "$tarball" -C "$tins_src" --strip-components 1 --no-same-owner
cd "$tins_src" && git apply "/root/tins-fix-cmake-config-install.patch"

tins_build () {
    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$tins_src" \
        -B "$tins_build_root" \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIBTINS_BUILD_TESTS=OFF \
        -DLIBTINS_BUILD_EXAMPLES=OFF \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$tins_build_root" --target install
}

tins_build /usr

rm -rf "$tins_root"
