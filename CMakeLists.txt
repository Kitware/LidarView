cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(LidarView)
#Do not rename Project, ${LidarView_SOURCE_DIR} ${LidarView_BINARY_DIR} are used, we could remove their usage though

# add path to get all the needed modules used to config Lidarview
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LidarView_SOURCE_DIR}/LVCore/CMake")

# we include the module that allow us to configure the Lidarview project
include(SetupLidarviewProject)

# custom code here
# ...

# Must be invoked Last
include(FinalizeLidarViewProject)
