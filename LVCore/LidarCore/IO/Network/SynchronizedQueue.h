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

#ifndef SYNCHRONIZEDQUEUE_H
#define SYNCHRONIZEDQUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

/**
 * @brief The SynchronizedQueue class is a FIFO structure whith some mutex to allow acces
 * from multiple thread.
 */
template<typename T>
class SynchronizedQueue
{
public:
  SynchronizedQueue()
    : queue_()
    , mutex_()
    , cond_()
    , request_to_end_(false)
    , enqueue_data_(true)
  {
  }

  void enqueue(const T &data)
  {
    std::unique_lock<std::mutex> lock(mutex_);

    if (enqueue_data_)
    {
      queue_.push(data);
      cond_.notify_one();
    }
  }

  bool dequeue(T &result)
  {
    std::unique_lock<std::mutex> lock(mutex_);

    while (queue_.empty() && (!request_to_end_))
    {
      cond_.wait(lock);
    }

    if (request_to_end_)
    {
      doEndActions();
      return false;
    }

    result = queue_.front();
    queue_.pop();

    return true;
  }

  void stopQueue()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    request_to_end_ = true;
    cond_.notify_one();
  }

  unsigned int size()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    return static_cast<unsigned int>(queue_.size());
  }

  bool isEmpty() const
  {
    std::unique_lock<std::mutex> lock(mutex_);
    return (queue_.empty());
  }

private:
  void doEndActions()
  {
    enqueue_data_ = false;

    while (!queue_.empty())
    {
      queue_.pop();
    }
  }

  std::queue<T> queue_;            // Use STL queue to store data
  mutable std::mutex mutex_;     // The mutex to synchronise on
  std::condition_variable cond_; // The condition to wait for

  bool request_to_end_;
  bool enqueue_data_;
};

#endif // SYNCHRONIZEDQUEUE_H
