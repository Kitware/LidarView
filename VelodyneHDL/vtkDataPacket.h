// Copyright 2016 Velodyne Lidar, Inc.
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

#ifndef _vtkDataPacket_h
#define _vtkDataPacket_h

#include <vector>
#include <iomanip>
#include <stdio.h>
#ifdef _MSC_VER
# include <boost/cstdint.hpp>
typedef boost::uint8_t uint8_t;
#else
# include <stdint.h>
#endif

namespace DataPacketFixedLength
{
#define HDL_Grabber_toRadians(x) ((x) * vtkMath::Pi() / 180.0)

static const int HDL_NUM_ROT_ANGLES = 36001;
static const int HDL_LASER_PER_FIRING = 32;
static const int HDL_MAX_NUM_LASERS = 128;
static const int HDL_FIRING_PER_PKT = 12;

enum HDLBlock
{
  BLOCK_0_TO_31 = 0xeeff,
  BLOCK_32_TO_63 = 0xddff,
  BLOCK_64_TO_95 = 0xccff,
  BLOCK_96_TO_127 = 0xbbff,
};

enum SensorType
{
  HDL32E = 0x21,
  VLP16  = 0x22,
  VLP32AB  = 0x23,
  VLP16HiRes  = 0x24,
  VLP32C  = 0x28,

  //Work around : this is not defined by any specification
  //But it is usefull to define
  HDL64  = 0xa0,
  VLP128  = 0xa1,
};
static int num_laser(SensorType sensorType){
  switch (sensorType)
  {
  case HDL64:
    return 64;
  case HDL32E:
  case VLP32AB:
  case VLP32C:
    return 32;
  case VLP16:
  case VLP16HiRes:
    return 16;
  case VLP128:
    return 128;
  default:
    return 0;
  }
}

enum DualReturnSensorMode
{
  STRONGEST_RETURN = 0x37,
  LAST_RETURN  = 0x38,
  DUAL_RETURN  = 0x39,
};
enum PowerMode
{
  NO_INTERNAL_CORRECTION_0 = 0xa0,
  NO_INTERNAL_CORRECTION_1 = 0xa1,
  NO_INTERNAL_CORRECTION_2 = 0xa2,
  NO_INTERNAL_CORRECTION_3 = 0xa3,
  NO_INTERNAL_CORRECTION_4 = 0xa4,
  NO_INTERNAL_CORRECTION_5 = 0xa5,
  NO_INTERNAL_CORRECTION_6 = 0xa6,
  NO_INTERNAL_CORRECTION_7 = 0xa7,
  CorrectionOn = 0xa8,
};

#pragma pack(push, 1)
typedef struct HDLLaserReturn
{
  uint16_t distance;
  uint8_t intensity;
} HDLLaserReturn;

struct HDLFiringData
{
  uint16_t blockIdentifier;
  uint16_t rotationalPosition;
  HDLLaserReturn laserReturns[HDL_LASER_PER_FIRING];

  inline bool isUpperBlock() const {
    return blockIdentifier==BLOCK_32_TO_63;
  }
};

struct HDLDataPacket
{
  HDLFiringData firingData[HDL_FIRING_PER_PKT];
  uint32_t gpsTimestamp;
  uint8_t factoryField1;
  uint8_t factoryField2;
  SensorType getSensorType() const
  {
    if (isVLP128());
      return VLP128;
    if (isHDL64())
      return HDL64;
    return static_cast<SensorType>(factoryField2);
  }
  DualReturnSensorMode getDualReturnSensorMode() const
  {
    if (isHDL64())
      return isHDL64DualModeReturn()?DUAL_RETURN:STRONGEST_RETURN;
    return static_cast<DualReturnSensorMode>(factoryField1);
  }
  static const unsigned int getDataByteLength()
  {
    return 1206;
  }
  static const unsigned int getPacketByteLength()
  {
    return getDataByteLength() + 42;
  }
  static inline bool isValidPacket(const unsigned char* data, unsigned int dataLength)
  {
    if (dataLength != getDataByteLength())
      return false;
    const HDLDataPacket* dataPacket = reinterpret_cast<const HDLDataPacket *>(data);
    return (dataPacket->firingData[0].blockIdentifier == BLOCK_0_TO_31
        || dataPacket->firingData[0].blockIdentifier == BLOCK_32_TO_63);
  }
  inline bool isHDL64() const {
    return firingData[1].isUpperBlock();
  }
  inline bool isVLP128() const {
    return firingData[2].blockIdentifier==BLOCK_64_TO_95
        && firingData[3].blockIdentifier==BLOCK_96_TO_127;
  }
  inline bool isDualModeReturn() const {
    if(isHDL64())
      return isHDL64DualModeReturn();
    else
      return firingData[1].rotationalPosition == firingData[0].rotationalPosition;
  }
  inline bool isHDL64DualModeReturn() const {
      return firingData[2].rotationalPosition == firingData[0].rotationalPosition;
  }
};

struct HDLLaserCorrection  // Internal representation of per-laser correction
{
  // In degrees
  double rotationalCorrection;
  double verticalCorrection;
  // In meters
  double distanceCorrection;
  double distanceCorrectionX;
  double distanceCorrectionY;

  double verticalOffsetCorrection;
  double horizontalOffsetCorrection;

  double focalDistance;
  double focalSlope;
  double closeSlope; // Used in HDL-64 only

  short minIntensity;
  short maxIntensity;

  // precomputed values
  double sinRotationalCorrection;
  double cosRotationalCorrection;
  double sinVertCorrection;
  double cosVertCorrection;
  double sinVertOffsetCorrection;
  double cosVertOffsetCorrection;
};

struct HDLRGB
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
#pragma pack(pop)
}// end namespace DataPacketFixedLength
#endif