FROM ubuntu:22.04
MAINTAINER Timothée Couble <timothee.couble@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_paraview.sh /root/install_paraview.sh
RUN sh /root/install_paraview.sh

COPY install_ceres.sh /root/install_ceres.sh
RUN sh /root/install_ceres.sh

COPY install_nanoflann.sh /root/install_nanoflann.sh
RUN sh /root/install_nanoflann.sh
