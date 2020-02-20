#include "PacketConsumer.h"

#include "SynchronizedQueue.h"

//----------------------------------------------------------------------------
PacketConsumer::PacketConsumer()
{
  this->Packets.reset(new SynchronizedQueue<NetworkPacket*>);
}

//----------------------------------------------------------------------------
void PacketConsumer::HandleSensorData(const unsigned char *data, unsigned int length)
{
  if (!this->Interpreter->IsValidPacket(data, length))
    return;
  this->Interpreter->ProcessPacket(data, length);
  if (this->Interpreter->IsNewData())
  {
    {
      boost::lock_guard<boost::mutex> lock(this->ConsumerMutex);
      this->Stream->AddNewData();
    }
    this->Stream->ClearAllDataAvailable();
  }
}

//----------------------------------------------------------------------------
void PacketConsumer::ThreadLoop()
{
  NetworkPacket* packet = nullptr;
  this->Interpreter->ResetCurrentData();
  while (this->Packets->dequeue(packet))
  {
    this->HandleSensorData(packet->GetPayloadData(), packet->GetPayloadSize());
    delete packet;
  }
}

//----------------------------------------------------------------------------
void PacketConsumer::Start()
{
  if (this->Thread)
  {
    return;
  }
  this->Interpreter->ResetCurrentData();

  this->Packets.reset(new SynchronizedQueue<NetworkPacket*>);
  this->Thread = boost::shared_ptr<boost::thread>(
        new boost::thread(boost::bind(&PacketConsumer::ThreadLoop, this)));
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
