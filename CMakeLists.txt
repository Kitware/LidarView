cmake_minimum_required(VERSION 3.20.3 FATAL_ERROR)
project(LidarView)
#Thanks to ExternalProject CMAKE_SOURCE_DIR will correctly be the same as this PROJECT_SOURCE_DIR

# add path to get all the needed modules used to config Lidarview
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/LVCore/CMake")

# Include branding.cmake
include(Application/branding.cmake)

# we include the module that allow us to configure the Lidarview project
include(SetupLidarviewProject)

# custom code here
add_subdirectory(Plugins/VelodynePlugin)

# Application
add_subdirectory(Application)

# Must be invoked Last
include(FinalizeLidarViewProject)
