/*=========================================================================

   Program: LidarView
   Module:  lqPythonQtCalibration.h

   Copyright (c) Kitware Inc.
   All rights reserved.

   LidarView is a free software; you can redistribute it and/or modify it
   under the terms of the LidarView license.

   See LICENSE for the full LidarView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
// Copyright 2013 Velodyne Acoustics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef lqPythonQtCalibration_h
#define lqPythonQtCalibration_h

#include <PythonQt.h>
#include <QObject>

#include "Widgets/vvCalibrationDialog.h"
#include "Widgets/vvCalibrationStructs.h"

// Otherwise PythonQT doesn't find the classes
using namespace vvCalibration;

class lqPythonQtCalibration : public QObject
{
  Q_OBJECT

public:
  lqPythonQtCalibration(QObject* parent = 0)
    : QObject(parent)
  {
    this->registerCPPClassForPythonQt("TransformConfig");
    this->registerCPPClassForPythonQt("NetworkConfig");
  }

  inline void registerCPPClassForPythonQt(const char* className)
  {
    PythonQt::self()->registerCPPClass(className, "", "paraview");
  }

  // Declare setter and getter to give access in Python
public Q_SLOTS:

  // TransformConfig constructor
  TransformConfig* new_TransformConfig() { return new TransformConfig(); };
  TransformConfig* new_TransformConfig(double yaw,
    double roll,
    double pitch,
    double x,
    double y,
    double z,
    double timeOffset)
  {
    return new TransformConfig({ yaw, roll, pitch, x, y, z, timeOffset });
  };

  // TransformConfig getter / setter
  double getYaw(TransformConfig* inst) { return inst->yaw; }
  double getRoll(TransformConfig* inst) { return inst->roll; }
  double getPitch(TransformConfig* inst) { return inst->pitch; }
  void setRotation(TransformConfig* inst, double yaw, double roll, double pitch)
  {
    inst->yaw = yaw;
    inst->roll = roll;
    inst->pitch = pitch;
  };
  double getX(TransformConfig* inst) { return inst->x; }
  double getY(TransformConfig* inst) { return inst->y; }
  double getZ(TransformConfig* inst) { return inst->z; }
  void setTranslation(TransformConfig* inst, double x, double y, double z)
  {
    inst->x = x;
    inst->y = y;
    inst->z = z;
  };
  double getTimeOffset(TransformConfig* inst) { return inst->timeOffset; }
  void setTimeOffset(TransformConfig* inst, double timeOffset) { inst->timeOffset = timeOffset; }

  // vvCalibrationDialog getter / setter for TransformConfig
  TransformConfig* getLidarConfig(vvCalibrationDialog* inst)
  {
    return new TransformConfig(inst->getLidarConfig());
  };
  void setLidarConfig(vvCalibrationDialog* inst, TransformConfig* conf)
  {
    inst->setLidarConfig(*conf);
  };
  TransformConfig* getGPSConfig(vvCalibrationDialog* inst)
  {
    return new TransformConfig(inst->getGPSConfig());
  };
  void setGPSConfig(vvCalibrationDialog* inst, TransformConfig* conf)
  {
    inst->setGPSConfig(*conf);
  };

  // NetworkConfig
  NetworkConfig* new_NetworkConfig() { return new NetworkConfig(); };
  NetworkConfig* new_NetworkConfig(int listenningPort,
    int forwardingPort,
    bool isForwarding,
    QString ipAddressForwarding,
    bool isCrashAnalysing)
  {
    return new NetworkConfig(
      { listenningPort, forwardingPort, isForwarding, ipAddressForwarding, isCrashAnalysing });
  };

  // NetworkConfig getter / setter
  int getListenningPort(NetworkConfig* inst) { return inst->listenningPort; };
  void setListenningPort(NetworkConfig* inst, int listenningPort)
  {
    inst->listenningPort = listenningPort;
  };
  int getForwardingPort(NetworkConfig* inst) { return inst->forwardingPort; };
  void setForwardingPort(NetworkConfig* inst, int forwardingPort)
  {
    inst->forwardingPort = forwardingPort;
  };
  bool getisForwarding(NetworkConfig* inst) { return inst->isForwarding; };
  void setisForwarding(NetworkConfig* inst, bool isForwarding)
  {
    inst->isForwarding = isForwarding;
  };
  QString getIpAddressForwarding(NetworkConfig* inst) { return inst->ipAddressForwarding; };
  void setIpAddressForwarding(NetworkConfig* inst, QString ipAddressForwarding)
  {
    inst->ipAddressForwarding = ipAddressForwarding;
  };
  int getIsCrashAnalysing(NetworkConfig* inst) { return inst->isCrashAnalysing; };
  void setIsCrashAnalysing(NetworkConfig* inst, int isCrashAnalysing)
  {
    inst->isCrashAnalysing = isCrashAnalysing;
  };

  // vvCalibrationDialog getter / setter for NetworkConfig
  NetworkConfig* getLidarNetworkConfig(vvCalibrationDialog* inst)
  {
    return new NetworkConfig(inst->getLidarNetworkConfig());
  };
  void setLidarNetworkConfig(vvCalibrationDialog* inst, NetworkConfig* conf)
  {
    inst->setLidarNetworkConfig(*conf);
  };
  NetworkConfig* getGPSNetworkConfig(vvCalibrationDialog* inst)
  {
    return new NetworkConfig(inst->getGPSNetworkConfig());
  };
  void setGPSNetworkConfig(vvCalibrationDialog* inst, NetworkConfig* conf)
  {
    inst->setGPSNetworkConfig(*conf);
  };
};

#endif // lqPythonQtCalibration_h