//=========================================================================
//
// Copyright 2020 Kitware, Inc.
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
//=========================================================================

// This file is not named "Time.h" to avoid confusions with "time.h"
#ifndef LVTIME_H
#define LVTIME_H

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include "LidarCoreModule.h"
#include <ctime>

double LIDARCORE_EXPORT GetElapsedTime(const timeval& now);

double LIDARCORE_EXPORT GetElapsedTime(const timeval& end, const timeval& start);

#endif // LVTIME_H
