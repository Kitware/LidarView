#===============================================================================
# Copyright 2021 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#===============================================================================

install(FILES "${ParaView_DIR}/bin/plugins/PythonQtPlugin/PythonQtPlugin.dll"
	      DESTINATION "bin"
        )

# install some python libs needed to launch Lidarview from the install dir
get_filename_component(python_lib_location ${Python3_LIBRARY} PATH)
install(DIRECTORY "${python_lib_location}/../Lib" 
        DESTINATION "bin"
        )

install(FILES "${python_lib_location}/../python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll" 
        DESTINATION "bin"
        )

foreach (qt5_opengl_lib IN ITEMS opengl32sw libEGL libGLESv2 libEGLd
  Qt5Core Qt5Gui Qt5Widgets Qt5Help Qt5Network Qt5PrintSupport Qt5Sql)
install(FILES "${Qt5_DIR}/../../../bin/${qt5_opengl_lib}.dll"
        DESTINATION "bin"
        )
endforeach ()
install(DIRECTORY "${Qt5_DIR}/../../../plugins/platforms"
        DESTINATION "bin"
        )
# see comment in: Superbuild/Projects/win32/lidarview.bundle.cmake
install(DIRECTORY "${Qt5_DIR}/../../../plugins/styles"
        DESTINATION "bin"
        )