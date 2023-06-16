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

#ifndef lqCalibrationStructs_h
#define lqCalibrationStructs_h

#include <QString>
#include <iostream>

namespace vvCalibration
{
enum class Plugin
{
  UNKNOWN,
  VELODYNE,
  HESAI
};

struct TransformConfig
{
  double yaw = 0.0;
  double roll = 0.0;
  double pitch = 0.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double timeOffset = 0.0;
};

struct NetworkConfig
{
  int listenningPort = 0;
  int forwardingPort = 0;
  bool isForwarding = false;
  QString ipAddressForwarding = "";
  bool isCrashAnalysing = false;
};
};

#endif /* !__lqCalibrationStructs_h */
