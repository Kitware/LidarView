/*=========================================================================

  Program: LidarView
  Module:  vtkPacketRecorder.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPacketRecorder.h"

#include "vtkSynchronizedQueue.h"

#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include <tins/tins.h>

namespace
{
unsigned int QUEUE_CACHE_SIZE = 500U;
using QueueType = vtkSynchronizedQueue<Tins::Packet>;

//-----------------------------------------------------------------------------
void StopThread(std::unique_ptr<std::thread>& thread)
{
  if (thread)
  {
    if (thread->joinable())
    {
      thread->join();
    }
    thread.reset();
  }
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPacketRecorder);

//-----------------------------------------------------------------------------
class vtkPacketRecorder::vtkInternals
{
public:
  std::unique_ptr<QueueType> DataQueue;

  //------------------------------------------------------------------------------
  void RecordingLoop(const std::string& filename)
  {
    std::unique_ptr<Tins::PacketWriter> tinsWriter =
      std::make_unique<Tins::PacketWriter>(filename, Tins::DataLinkType<Tins::EthernetII>());

    Tins::Packet packet;
    while (this->DataQueue->Dequeue(packet))
    {
      tinsWriter->write(packet);
    }
  }
};

//------------------------------------------------------------------------------
vtkPacketRecorder::vtkPacketRecorder()
  : Internals(new vtkPacketRecorder::vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkPacketRecorder::~vtkPacketRecorder()
{
  this->StopRecording();
}

//-----------------------------------------------------------------------------
void vtkPacketRecorder::StartRecording()
{
  if (this->RecordingFileName.empty())
  {
    return;
  }
  auto& internals = *this->Internals;
  internals.DataQueue = std::make_unique<QueueType>(::QUEUE_CACHE_SIZE);
  this->WritingThread =
    std::make_unique<std::thread>([&] { internals.RecordingLoop(this->RecordingFileName); });
  this->IsRecording = true;
}

//-----------------------------------------------------------------------------
void vtkPacketRecorder::StopRecording()
{
  this->IsRecording = false;
  if (this->Internals->DataQueue)
  {
    this->Internals->DataQueue->StopQueue();
  }
  ::StopThread(this->WritingThread);
}

//----------------------------------------------------------------------------
void vtkPacketRecorder::AddPacketToWritingQueue(const Tins::Packet* pkt)
{
  auto& internals = *this->Internals;
  if (this->IsRecording && this->WritingThread && internals.DataQueue)
  {
    internals.DataQueue->Enqueue(*pkt);
  }
}
