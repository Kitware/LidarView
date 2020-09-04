# Please use https links whenever possible because some people
# cannot clone using ssh (git://) due to a firewalled network.
# This maintains the links for all sources used by this superbuild.
# Simply update this file to change the revision.
# One can use different revision on different platforms.
# e.g.
# if (UNIX)
#   ..
# else (APPLE)
#   ..
# endif()

superbuild_set_revision(pythonqt
	GIT_REPOSITORY https://gitee.com/debupt/PythonQt.git
	GIT_TAG patched-8)

set(PARAVIEW_VERSION 5.4)
superbuild_set_revision(paraview
	URL "http://0.0.0.0:8000/paraview.tar.gz"
	URL_MD5 90f2d2a593643526ae0f69722a5268bb)

superbuild_set_revision(lidarview
		SOURCE_DIR ${CMAKE_SOURCE_DIR}/..
		DOWNLOAD_COMMAND "")

if (WIN32)
	superbuild_set_revision(pcap
		GIT_REPOSITORY http://github.com/patmarion/winpcap.git
		GIT_TAG master)
else()
	superbuild_set_revision(pcap
		URL "http://0.0.0.0:8000/libpcap-1.4.0.tar.gz"
		URL_MD5 56e88a5aabdd1e04414985ac24f7e76c)
endif()

# General
# another revision of boost is addedd inside Superbuild/common-superbuild/versions.txt
# but this file has a higher priority
superbuild_set_revision(boost
	URL "http://0.0.0.0:8000/boost_1_63_0.tar.gz"
	URL_MD5 7b493c08bc9557bbde7e29091f28b605)

superbuild_set_revision(eigen
	GIT_REPOSITORY https://gitee.com/debupt/eigen-git-mirror.git
	GIT_TAG 3.2.10)

superbuild_set_revision(liblas
	URL "http://0.0.0.0:8000/libLAS-1.8.1.tar.bz2"
	URL_MD5 2e6a975dafdf57f59a385ccb87eb5919)
	
superbuild_set_revision(ceres
	URL "http://0.0.0.0:8000/ceres-solver.tar.gz"
	URL_MDS 371358376f085e05eb2a815e2e9040f2)

superbuild_set_revision(glog
	GIT_REPOSITORY https://gitee.com/debupt/glog.git
	GIT_TAG 8d7a107d68c127f3f494bb7807b796c8c5a97a82)

superbuild_set_revision(pcl
	GIT_REPOSITORY https://gitee.com/debupt/pcl.git
	GIT_TAG pcl-1.8.1)

superbuild_set_revision(qhull
		GIT_REPOSITORY https://gitee.com/debupt/qhull.git
		GIT_TAG master)

superbuild_set_revision(flann
	GIT_REPOSITORY https://gitee.com/debupt/flann.git
	GIT_TAG 1.9.1)

superbuild_set_revision(opencv
	URL "http://0.0.0.0:8000/opencv.tar.gz"
	URL_MD5 09002733bcd52b97cdd870f1262f8b6c)

superbuild_set_revision(nanoflann
	GIT_REPOSITORY https://gitee.com/debupt/nanoflann.git
	GIT_TAG v1.3.0)

superbuild_set_revision(yaml
	GIT_REPOSITORY https://gitee.com/debupt/yaml-cpp.git
	GIT_TAG yaml-cpp-0.6.2)

superbuild_set_revision(darknet
	GIT_REPOSITORY https://gitee.com/debupt/darknet.git
	GIT_TAG master)
