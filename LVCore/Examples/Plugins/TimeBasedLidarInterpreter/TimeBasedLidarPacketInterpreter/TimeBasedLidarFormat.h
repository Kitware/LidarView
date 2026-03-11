/*=========================================================================

  Program:   LidarView
  Module:    TimeBasedLidarFormat.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef TimeBasedLidarFormat_h
#define TimeBasedLidarFormat_h

#include <cstdint>

constexpr unsigned int NB_OF_POINTS = 100U;

// Instructs the compiler to pack structure members
#pragma pack(push, 1)

struct PointType
{
  float x;
  float y;
  float z;
  float reflectance;
  int64_t timestamp; // in nanoseconds
};

struct PacketType
{
  PointType data[NB_OF_POINTS];
};

#pragma pack(pop)

#endif // TimeBasedLidarFormat_h
