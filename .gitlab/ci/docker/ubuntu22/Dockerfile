FROM ubuntu:22.04
LABEL maintainer="Timothée Couble <timothee.couble@kitware.com>"

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY install_paraview.sh /root/install_paraview.sh
RUN sh /root/install_paraview.sh

COPY install_ceres.sh /root/install_ceres.sh
RUN sh /root/install_ceres.sh

COPY install_nanoflann.sh /root/install_nanoflann.sh
RUN sh /root/install_nanoflann.sh

COPY install_tins.sh /root/install_tins.sh
COPY tins-fix-cmake-config-install.patch /root/tins-fix-cmake-config-install.patch
RUN sh /root/install_tins.sh
