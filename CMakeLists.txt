cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(LidarView)

# add path to get all the needed modules used to config Lidarview
set(LidarView_CMAKE_DIR "${LidarView_SOURCE_DIR}/LVCore/CMake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LidarView_CMAKE_DIR})

# we include the module that allow us to configure the Lidarview project
include(SetupLidarviewProject)
