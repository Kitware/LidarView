#!/bin/sh

set -e

dnf install -y --setopt=install_weak_deps=False \
  glx-utils mesa-dri-drivers mesa-libGL* libxcrypt-compat.x86_64 libxkbcommon \
  libxkbcommon-x11

# Install xvfb for offline graphical testing
dnf install -y --setopt=install_weak_deps=False \
    xorg-x11-server-Xvfb

# Install extra dependencies for ParaView
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch doxygen git-core git-lfs

# Qt dependencies
dnf install -y --setopt=install_weak_deps=False \
    qt5-qtbase-devel qt5-qtbase-private-devel qt5-qttools-devel qt5-qtsvg-devel \
    qt5-qtxmlpatterns-devel qt5-qtmultimedia-devel qt5-qttools-static

# GNOME theme requirements
dnf install -y --setopt=install_weak_deps=False \
    abattis-cantarell-fonts

# Development tools
dnf install -y --setopt=install_weak_deps=False \
    libasan libtsan libubsan clang-tools-extra \
    gcc gcc-c++ gcc-gfortran \
    ninja-build

# External dependencies
dnf install -y --setopt=install_weak_deps=False \
    libXcursor-devel libharu-devel utf8cpp-devel pugixml-devel libtiff-devel \
    eigen3-devel double-conversion-devel glew-devel jsoncpp-devel boost-devel \
    libpcap-devel gdal-devel PDAL-devel pcl-devel yaml-cpp-devel

# Python dependencies
dnf install -y --setopt=install_weak_deps=False \
    python3-twisted python3-autobahn python3 python3-devel python3-numpy \
    python3-pandas python3-pandas-datareader python3-sphinx python3-pip \
    python3-mpi4py-mpich python3-mpi4py-openmpi python3-matplotlib

python3 -m pip install wslink cftime

dnf clean all
