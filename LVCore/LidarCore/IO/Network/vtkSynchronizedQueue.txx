/*=========================================================================

  Program: LidarView
  Module:  vtkSynchronizedQueue.txx

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSynchronizedQueue_txx
#define vtkSynchronizedQueue_txx

#include "vtkSynchronizedQueue.h"

//-----------------------------------------------------------------------------
template <typename T>
vtkSynchronizedQueue<T>::vtkSynchronizedQueue(unsigned int maxCacheSize)
  : MaxQueueSize(maxCacheSize)
{
}

//-----------------------------------------------------------------------------
template <typename T>
void vtkSynchronizedQueue<T>::Enqueue(const T& data)
{
  std::unique_lock<std::mutex> lock(this->DataMutex);

  if (this->IsEnqueuingData)
  {
    this->Queue.push(data);
    this->SyncCondition.notify_one();
  }

  if (this->Queue.size() >= this->MaxQueueSize)
  {
    this->Queue.pop();
  }
}

//-----------------------------------------------------------------------------
template <typename T>
bool vtkSynchronizedQueue<T>::Dequeue(T& data)
{
  std::unique_lock<std::mutex> lock(this->DataMutex);

  while (this->Queue.empty() && (!this->RequestToEnd))
  {
    this->SyncCondition.wait(lock);
  }

  if (this->RequestToEnd)
  {
    this->IsEnqueuingData = false;
    while (!this->Queue.empty())
    {
      this->Queue.pop();
    }
    return false;
  }

  data = this->Queue.front();
  this->Queue.pop();

  return true;
}

//-----------------------------------------------------------------------------
template <typename T>
void vtkSynchronizedQueue<T>::StopQueue()
{
  std::unique_lock<std::mutex> lock(this->DataMutex);
  this->RequestToEnd = true;
  this->SyncCondition.notify_one();
}

#endif
