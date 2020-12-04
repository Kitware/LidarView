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

#ifndef PACKETCONSUMER_H
#define PACKETCONSUMER_H

#include <memory>
#include <thread>
#include <mutex>

template<typename T>
class SynchronizedQueue;
class NetworkPacket;
class vtkStream;

class PacketConsumer
{
public:
  PacketConsumer(vtkStream * stream);
  ~PacketConsumer();

  void HandleSensorData(NetworkPacket* packet);

  void Start();

  void Stop();

  void Enqueue(NetworkPacket* packet);

protected:
  void ThreadLoop();

  vtkStream * Stream;

  std::unique_ptr<SynchronizedQueue<NetworkPacket*>> Packets;
  /*!< Number of packets to cache, above: drop oldest packets */
  size_t PacketCacheSize = 100000;

  std::unique_ptr<std::thread> Thread;
};

#endif // PACKETCONSUMER_H
