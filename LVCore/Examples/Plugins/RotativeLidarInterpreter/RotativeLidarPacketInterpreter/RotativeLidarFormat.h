/*=========================================================================

  Program:   LidarView
  Module:    RotativeLidarFormat.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef RotativeLidarFormat_h
#define RotativeLidarFormat_h

#include <boost/endian/arithmetic.hpp>
#include <boost/predef/other/endian.h>

#include <memory>

#include "InterpreterHelper.h"

static constexpr unsigned int numberOfChannels{ 64 };

#pragma pack(push, 1)
//! @brief class representing the Example packet header
/*
   0               1               2               3
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                            FrameID                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Timestamp                           |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | InfoA  | InfoB |                 Reserved                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
class PacketHeader
{
private:
  // Frame ID
  boost::endian::big_uint32_t FrameID;

  // Timestamp
  boost::endian::big_uint64_t Timestamp;

  // 4 bits InfoA
  // 4 bits InfoB
  boost::endian::big_uint8_t InfoA_InfoB;

  // Reserved
  boost::endian::big_uint32_t Reserved;

public:
  GET_NATIVE_UINT(32, FrameID)
  SET_NATIVE_UINT(32, FrameID)
  GET_NATIVE_UINT(64, Timestamp)
  SET_NATIVE_UINT(64, Timestamp)
  GET_NATIVE_UINT(32, Reserved)
  SET_NATIVE_UINT(32, Reserved)

  uint8_t GetInfoB() const { return BF_GET(InfoA_InfoB, 0, 4); }

  uint8_t GetInfoA() const { return BF_GET(InfoA_InfoB, 4, 4); }
};
#pragma pack(pop)

#pragma pack(push, 1)
//! @brief class representing the Example firing return
/*
   0               1               2               3
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |    ChannelID   | Reflectivity  |            Azimuth           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             Distance           |   Intensity   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
 */
class FiringBlock
{
private:
  // Channel ID
  boost::endian::big_uint8_t ChannelID;

  // Reflectivity
  boost::endian::big_uint8_t Reflectivity;

  // Azimuth
  boost::endian::big_uint16_t Azimuth;

  // Distance of the return
  boost::endian::big_uint16_t Distance;

  // Intensity
  boost::endian::big_uint8_t Intensity;

public:
  GET_NATIVE_UINT(8, ChannelID)
  SET_NATIVE_UINT(8, ChannelID)
  GET_NATIVE_UINT(8, Reflectivity)
  SET_NATIVE_UINT(8, Reflectivity)
  GET_NATIVE_UINT(16, Azimuth)
  SET_NATIVE_UINT(16, Azimuth)
  GET_NATIVE_UINT(16, Distance)
  SET_NATIVE_UINT(16, Distance)
  GET_NATIVE_UINT(8, Intensity)
  SET_NATIVE_UINT(8, Intensity)
};
#pragma pack(pop)

#pragma pack(push, 1)
//! @brief class representing the Example Packet
struct LidarPacket
{
  PacketHeader header;
  FiringBlock firings[64];
};
#pragma pack(pop)

#endif // RotativeLidarFormat_h
