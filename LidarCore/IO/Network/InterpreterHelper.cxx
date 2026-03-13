/*=========================================================================

  Program: LidarView
  Module:  InterpreterHelper.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "InterpreterHelper.h"

#include "time.h"

#if defined(_WIN32)
#define timegm _mkgmtime
#endif

//------------------------------------------------------------------------------
uint64_t ConvertUTCToTimestamp(std::vector<uint8_t> utcTime, bool hasMonthOffset)
{
  if (utcTime.size() != 6)
  {
    return 0;
  }

  struct tm t;
  t.tm_year = utcTime[0];
  t.tm_mon = utcTime[1] - static_cast<uint8_t>(hasMonthOffset);
  t.tm_mday = utcTime[2];
  t.tm_hour = utcTime[3];
  t.tm_min = utcTime[4];
  t.tm_sec = utcTime[5];
  t.tm_isdst = 0;
  time_t seconds = timegm(&t);
  return seconds;
}
