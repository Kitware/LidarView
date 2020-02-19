# Base Image
FROM ubuntu:16.04

WORKDIR /builds/

# Install dependancies
# libxkbcommon-dev, libxcb-xkb-dev, libxslt1-dev and libgstreamer-plugins-base1.0-dev
# are required to handle an xkb error see https://nicroland.wordpress.com/2015/12/06/running-qtcreator-in-docker/
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        libxkbcommon-dev \
        libxcb-xkb-dev \
        libxslt1-dev \
        libgstreamer-plugins-base1.0-dev \
        cmake \
        git \
        flex \
        byacc \
        python-minimal \
        python2.7-dev \
        libxext-dev \
        libxt-dev \
        libbz2-dev \
        zlib1g-dev \
        freeglut3-dev \
        pkg-config && \
    apt-get autoremove

COPY . lidarview

WORKDIR /superbuild

RUN cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DBUILD_TESTING:BOOL=ON \
    /builds/lidarview/Superbuild

RUN make -j16
