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
#ifndef __vvConfigure_h
#define __vvConfigure_h

#if defined(_WIN32)

#if defined(LidarPlugin_EXPORTS)
#define LidarPlugin_EXPORT __declspec(dllexport)
#else
#define LidarPlugin_EXPORT __declspec(dllimport)
#endif

#if defined(LidarPythonQT_EXPORTS)
#define LidarPythonQT_EXPORT __declspec(dllexport)
#else
#define LidarPythonQT_EXPORT __declspec(dllimport)
#endif

#else
#define LidarPlugin_EXPORT
#define LidarPythonQT_EXPORT
#endif

#endif // __vvConfigure_h
