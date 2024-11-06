/*=========================================================================

  Program: LidarView
  Module:  vtkSynchronizedQueue.h

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSynchronizedQueue_h
#define vtkSynchronizedQueue_h

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

/**
 * The vtkSynchronizedQueue class is a thread-safe FIFO structure
 * that can be accessed from multiple threads simultaneously.
 */
template <typename T>
class vtkSynchronizedQueue
{
public:
  /**
   * Create a new queue with a specified maximum number of elements.
   */
  vtkSynchronizedQueue(unsigned int maxCacheSize);

  ///@{
  /**
   * Add / pop elements from the queue.
   * Return false if stopping queue is requested.
   */
  bool Enqueue(const T& data);
  bool Dequeue(T& data);
  ///@}

  /**
   * Stops further enqueuing of new data and clears the current queue.
   *
   * Note that once the queue is stopped, it cannot be restarted. A new instance of
   * the queue must be created.
   */
  void StopQueue();

private:
  std::queue<T> Queue;
  std::mutex DataMutex;
  std::condition_variable SyncCondition;

  bool RequestToEnd = false;
  bool IsEnqueuingData = true;
  unsigned int MaxQueueSize;
};

#include "vtkSynchronizedQueue.txx" // for template implementations

#endif
