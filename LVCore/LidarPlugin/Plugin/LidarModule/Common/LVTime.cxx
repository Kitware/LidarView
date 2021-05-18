#include "Common/LVTime.h"

double GetElapsedTime(const timeval& now)
{
  struct timeval StartTime;
  StartTime.tv_sec = 0;
  StartTime.tv_usec = 0;
  return GetElapsedTime(now, StartTime);
}

double GetElapsedTime(const timeval& end, const timeval& start)
{
  return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}
