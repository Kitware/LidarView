#include "PacketConsumer.h"

#include <functional>
#include <cassert>

#include "NetworkPacket.h"
#include "SynchronizedQueue.h"
#include "IO/vtkInterpreter.h"
#include "IO/vtkStream.h"
#include "Common/LVTime.h"

//----------------------------------------------------------------------------
PacketConsumer::PacketConsumer(vtkStream* stream)
{
  this->Stream = stream;
}

//----------------------------------------------------------------------------
PacketConsumer::~PacketConsumer()
{
  this->Stop();
}

//----------------------------------------------------------------------------
void PacketConsumer::HandleSensorData(NetworkPacket* packet)
{
  const unsigned char *data = packet->GetPayloadData();
  if(!data)
  {
    return;
  }
  const unsigned int length = packet->GetPayloadSize();
  vtkInterpreter * interp = this->Stream->GetInterpreter();
  if (!interp->IsValidPacket(data, length))
    return;

  interp->ProcessPacketWrapped(data, length, GetElapsedTime(packet->ReceptionTime));
  if (interp->IsNewData())
  {
    {
      std::lock_guard<std::mutex> lock(this->Stream->DataMutex);
      this->Stream->AddNewData();
    }
    this->Stream->ClearAllDataAvailable();
  }
}

//----------------------------------------------------------------------------
void PacketConsumer::ThreadLoop()
{
  NetworkPacket* packet = nullptr;
  this->Stream->GetInterpreter()->ResetCurrentData();
  while (this->Packets->dequeue(packet))
  {
    this->HandleSensorData(packet);
    delete packet;
  }
}

//----------------------------------------------------------------------------
void PacketConsumer::Start()
{
  this->Stop();;
  this->Stream->GetInterpreter()->ResetCurrentData();
  this->Packets = std::make_unique<SynchronizedQueue<NetworkPacket*>>();
  this->Thread = std::make_unique<std::thread>(std::bind(&
                                                         PacketConsumer::ThreadLoop, this));
}

//----------------------------------------------------------------------------
void PacketConsumer::Stop()
{
  if (this->Thread)
  {
    this->Packets->stopQueue();
    this->Thread->join();
    this->Thread.reset();
    this->Packets.reset();
  }
}

//----------------------------------------------------------------------------
void PacketConsumer::Enqueue(NetworkPacket* packet)
{
  assert(this->Packets && "The consumer need to be started before enqueueing");
  this->Packets->enqueue(packet);

  // In order to prevent memory usage to grow unbounded, we limit the growth of
  // the packet cache. Above an arbitrary limit it seems safe to assume that
  // the lateness of the consuming (decoding) thread has zero chance of being
  // catched up and that packets can be dropped.
  // To test: look at memory usage while running PacketFileSender --speed 100
  if (this->Packets->size() > this->PacketCacheSize)
  {
      NetworkPacket* packetToDiscard = nullptr;
      this->Packets->dequeue(packetToDiscard);
      delete packetToDiscard;
  }
}
