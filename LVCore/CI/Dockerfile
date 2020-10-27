# Base Image
FROM ubuntu:16.04

WORKDIR /builds/

# Install dependancies
# libxkbcommon-dev, libxcb-xkb-dev, libxslt1-dev and libgstreamer-plugins-base1.0-dev
# are required to handle an xkb error see https://nicroland.wordpress.com/2015/12/06/running-qtcreator-in-docker/
# curl is needed for git-lfs latter on
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        libxkbcommon-dev \
        libxcb-xkb-dev \
        libxslt1-dev \
        libgstreamer-plugins-base1.0-dev \
        curl \
        cmake \
        git \
        flex \
        byacc \
        python-minimal \
        python3.7-dev \
        libxext-dev \
        libxt-dev \
        libbz2-dev \
        zlib1g-dev \
        freeglut3-dev \
        pkg-config && \
    apt-get autoremove

# git-lfs is only available on packagecloud.io for ubuntu 16.04
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash
RUN apt install git-lfs

COPY . lidarview

WORKDIR /superbuild

RUN cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DBUILD_TESTING:BOOL=ON \
    /builds/lidarview/Superbuild

RUN make -j16
