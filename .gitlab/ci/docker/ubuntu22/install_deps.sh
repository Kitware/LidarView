#!/bin/sh

set -e

apt update

# Install xvfb for offline graphical testing
apt install -y --no-install-recommends \
    xvfb libxcursor1

# Install extra dependencies for ParaView
apt install -y --no-install-recommends \
    libglvnd-dev bzip2 patch doxygen git git-lfs

# Qt extra depency
DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    tzdata

# Qt dependencies
apt install -y --no-install-recommends \
    qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qtmultimedia5-dev \
    qtbase5-private-dev libqt5x11extras5-dev libqt5svg5-dev qttools5-dev \
    libqt5help5 qtmultimedia5-dev qtbase5-private-dev libqt5x11extras5-dev \
    libqt5svg5-dev libqt5xmlpatterns5 qttools5-dev qtxmlpatterns5-dev-tools

# Development tools
apt install -y --no-install-recommends \
    clang-tools clang-tidy clang-format cmake cmake-curses-gui \
    gcc g++ ninja-build curl cppcheck

# External dependencies
apt install -y --no-install-recommends \
    libpugixml-dev libtiff-dev libeigen3-dev libdouble-conversion-dev libglew-dev \
    libjsoncpp-dev libboost-dev libpcap-dev libgdal-dev libpdal-dev libpcl-dev \
    libyaml-cpp-dev

# Ceres dependencies
apt install -y --no-install-recommends \
    libgflags-dev libgoogle-glog-dev libatlas-base-dev

# Python dependencies
apt install -y --no-install-recommends \
    python3-twisted python3-autobahn python3 python3-dev python3-numpy \
    python3-pandas python3-sphinx python3-pip python3-matplotlib

python3 -m pip install wslink cftime json-spec

apt clean all
