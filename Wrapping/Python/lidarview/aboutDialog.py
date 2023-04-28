# Copyright 2017 Velodyne Acoustics, Inc.
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
from __future__ import print_function
from PythonQt import QtCore, QtGui, QtUiTools

def showDialog(mainWindow):

    loader = QtUiTools.QUiLoader()
    uifile = QtCore.QFile(':/vvResources/vvAboutDialog.ui')
    if not uifile.open(uifile.ReadOnly):
        print("Canno't open the ui for the about dialog.")
        return

    dialog = loader.load(uifile, mainWindow)
    uifile.close()
    dialog.setModal(True)

    # Delete "?" Button that appears on windows os
    # Rewrite the flags without QtCore.Qt.WindowContextHelpButtonHint
    flags = QtCore.Qt.Dialog | QtCore.Qt.WindowStaysOnTopHint | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowCloseButtonHint
    dialog.setWindowFlags(flags)

    def w(name):
        for widget in dialog.children():
            if widget.objectName == name:
                return widget

    image = w('splashLabel')
    splash = image.pixmap.scaled(image.pixmap.width()/2.0, image.pixmap.height()/2.0)
    image.setPixmap(splash)
    image.adjustSize()

    appName = mainWindow.windowTitle.split(" ")[0]
    appVersionTag = mainWindow.windowTitle.split(" ")[1]
    appBitTag = mainWindow.windowTitle.split(" ")[2]
    dialog.windowTitle = "About " + appName + " ..."
    copyrightText = '''<h1>{0} {1} {2}</h1><br/>
                       Copyright (c) 2013-2017, Velodyne Lidar,
                       Copyright (c) 2016-2022, Kitware<br />
                       Provided by <a href="http://www.lidarview.org">Your company name</a>, coded by <a href="https://www.kitware.com">Kitware</a>.<br />
                       <br />
                    '''.format(appName, appVersionTag, appBitTag)
    w('copyrightLabel').setText(copyrightText)
    
    textBoxContent = '''<h4>Want more? Ask Kitware!</h4>
                        Kitware is a leading provider of open-source software systems for technical and scientific computing.
                        We are the developers of LidarView, providing real-time interactive visualization of live captured 3D LiDAR
                        data from Lidar sensors. We create customized solutions providing detection and tracking of people,
                        street signs, lane markings, vehicles, industrial machinery, and building facades from within LidarView or using
                        combinations of point cloud and video data. We also provide Lidar-based SLAM algorithms for Lidar integrators.
                        We work with customers to create tailored solutions using proven open-source
                        technologies, avoiding vendor lock-in and leveraging our world-leading experience in visualization, computer vision, high-performance
                        computing, and test-driven high-quality software process.<br />
                        <br />
                        <br />
                        Have a look at <a href="https://www.kitware.com/our-expertise/">our expertise</a>, and for more information, please contact us: 
                        <a href="mailto:kitware@kitware.fr?subject=Contact+about+LidarView">kitware@kitware.fr</a>
                     '''
    w('detailsLabel').setText(textBoxContent)
    
    
    button = w('closeButton')
    closeIcon = QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DialogCloseButton)
    button.setIcon(closeIcon)
    button.connect(button, QtCore.SIGNAL('clicked()'), dialog, QtCore.SLOT('close()'))
    dialog.adjustSize()
    dialog.setFixedSize(dialog.size)

    dialog.exec_()
