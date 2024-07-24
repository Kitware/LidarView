/*=========================================================================

  Program: LidarView
  Module:  vtkStream.cxx

  Copyright 2013 Velodyne Acoustics, Inc.
  Copyright 2018 (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStream.h"

#include <vtksys/SystemTools.hxx>

#include "PacketConsumer.h"
#include "PacketFileWriter.h"
#include "PacketReceiver.h"

//-----------------------------------------------------------------------------
vtkStream::vtkStream()
{
  // cannot use default constructor in header "vtkStream() = default" due to forward declaration of
  // some class
}

//-----------------------------------------------------------------------------
vtkStream::~vtkStream() {}

//-----------------------------------------------------------------------------
vtkMTimeType vtkStream::GetMTime()
{
  if (this->GetInterpreter())
  {
    return std::max(this->Superclass::GetMTime(), this->GetInterpreter()->GetMTime());
  }
  return this->Superclass::GetMTime();
}

//-----------------------------------------------------------------------------
template <class T>
void vtkStream::SetAttributeAndRestartIfRunning(T& attribute, const T& value)
{
  if (attribute != value)
  {
    attribute = value;
    if (this->IsRunning())
    {
      this->Stop();
      this->Start();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkStream::SetForwardedIpAddress(const std::string& value)
{
  SetAttributeAndRestartIfRunning(this->ForwardedIpAddress, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetListeningPort(int value)
{
  SetAttributeAndRestartIfRunning(this->ListeningPort, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetMulticastAddress(const std::string& value)
{
  SetAttributeAndRestartIfRunning(this->MulticastAddress, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetLocalListeningAddress(const std::string& value)
{
  // TODO may need to soft comparaison
  SetAttributeAndRestartIfRunning(this->LocalListeningAddress, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetForwardedPort(int value)
{
  SetAttributeAndRestartIfRunning(this->ForwardedPort, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetIsForwarding(bool value)
{
  SetAttributeAndRestartIfRunning(this->IsForwarding, value);
}

//-----------------------------------------------------------------------------
void vtkStream::SetIsCrashAnalysing(bool value)
{
  SetAttributeAndRestartIfRunning(this->IsCrashAnalysing, value);
}

//-----------------------------------------------------------------------------
bool vtkStream::GetNeedsUpdate()
{
  if (this->CheckForNewData())
  {
    this->UpdateLiveSource();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
int vtkStream::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  this->Start();
  return 1;
}

//----------------------------------------------------------------------------
void vtkStream::Start()
{
  if (!this->Interpreter)
  {
    vtkErrorMacro("no interpreter is set");
    return;
  }

  // Stop the stream, to handle successing call to Start()
  //************************************
  this->Stop();

  // Create the ConsumerThread
  //************************************
  this->ConsumerThread = std::make_unique<PacketConsumer>(this);

  // Create the ReceiverThread
  //**********************************
  this->ReceiverThread = std::make_unique<PacketReceiver>(this->ListeningPort,
    std::bind(&vtkStream::EnqueuePacket, this, std::placeholders::_1),
    this->MulticastAddress,
    this->LocalListeningAddress);
  if (this->IsForwarding)
  {
    this->ReceiverThread->EnableForwarding(this->ForwardedPort, this->ForwardedIpAddress);
  }
  if (this->IsCrashAnalysing)
  {
    std::string appDir = vtksys::SystemTools::GetCurrentWorkingDirectory();
    if (!appDir.empty())
    {
      vtkErrorMacro("Unable to get Crash Analysis file");
    }
    this->ReceiverThread->EnableCrashAnalysing(appDir, 5000);
  }
  this->ReceiverThread->SetFakeManufacturerMACAddress(
    this->Interpreter->GetManufacturerMACAddress());

  // Start the different threads
  //************************************
  // The starting order matters, as the receiver thread will enqueue packets on the consumer thread
  this->ConsumerThread->Start();
  this->ReceiverThread->Start();
}

//----------------------------------------------------------------------------
void vtkStream::Stop()
{
  // destruct the thread is the inverse order of creation
  if (this->ReceiverThread)
  {
    this->ReceiverThread->Stop();
    this->ReceiverThread.reset();
  }
  if (this->ConsumerThread)
  {
    this->ConsumerThread->Stop();
    this->ConsumerThread.reset();
  }
}

//-----------------------------------------------------------------------------
void vtkStream::StartRecording()
{
  this->StartRecording(std::make_shared<PacketFileWriter>());
}

//-----------------------------------------------------------------------------
void vtkStream::StartRecording(std::shared_ptr<PacketFileWriter> writer)
{
  if (this->RecordingFilename.empty())
  {
    return;
  }
  this->StopRecording();
  this->WriterThread = writer;
  this->WriterThread->Start(this->RecordingFilename);
}

//-----------------------------------------------------------------------------
void vtkStream::StopRecording()
{
  if (this->WriterThread)
  {
    this->WriterThread->Stop();
    this->WriterThread.reset();
  }
}

//-----------------------------------------------------------------------------
bool vtkStream::IsRecording()
{
  return this->WriterThread ? true : false;
}

//-----------------------------------------------------------------------------
void vtkStream::EnqueuePacket(NetworkPacket* packet)
{
  NetworkPacket* packet2 = nullptr;
  if (this->WriterThread)
  {
    packet2 = new NetworkPacket(*packet);
    this->WriterThread->Enqueue(packet2);
  }
  assert(this->ConsumerThread && "The receiver thread should be started before the consumer one");
  this->ConsumerThread->Enqueue(packet);
}

//-----------------------------------------------------------------------------
bool vtkStream::IsRunning()
{
  return this->ReceiverThread && this->ConsumerThread;
}
