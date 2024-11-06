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

#include "vtkUDPPacketReceiver.h"

//-----------------------------------------------------------------------------
vtkStream::vtkStream() = default;

//-----------------------------------------------------------------------------
vtkStream::~vtkStream() = default;

//-----------------------------------------------------------------------------
vtkMTimeType vtkStream::GetMTime()
{
  if (this->GetPacketHandler())
  {
    return std::max(this->Superclass::GetMTime(), this->GetPacketHandler()->GetMTime());
  }
  return this->Superclass::GetMTime();
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
  this->Start({ this->ListeningPort });
}

//----------------------------------------------------------------------------
void vtkStream::Start(const std::vector<unsigned int>& ports)
{
  if (this->PacketHandler)
  {
    auto consumeCallback = [this](const std::vector<uint8_t>& pkt, double timestamp)
    { this->ConsumePacket(pkt, timestamp); };
    this->PacketHandler->StartListening(ports, consumeCallback);
  }
}

//----------------------------------------------------------------------------
void vtkStream::Stop()
{
  if (this->PacketHandler)
  {
    this->PacketHandler->StopListening();
  }
}

//----------------------------------------------------------------------------
void vtkStream::SetPacketHandler(vtkStreamPacketHandler* handler)
{
  if (handler != this->PacketHandler)
  {
    bool isListening = false;
    if (this->PacketHandler)
    {
      isListening = this->PacketHandler->IsListening();
      this->PacketHandler->StopListening();
    }

    this->PacketHandler = handler;
    this->Modified();

    if (isListening)
    {
      this->Start();
    }
  }
}

//----------------------------------------------------------------------------
void vtkStream::SetForwardedPort(int port)
{
  if (vtkUDPPacketReceiver* receiver = vtkUDPPacketReceiver::SafeDownCast(this->PacketHandler))
  {
    return receiver->SetForwardedPortOffset(port - this->ListeningPort);
  }
};

//----------------------------------------------------------------------------
int vtkStream::GetForwardedPort()
{
  if (vtkUDPPacketReceiver* receiver = vtkUDPPacketReceiver::SafeDownCast(this->PacketHandler))
  {
    return this->ListeningPort + receiver->GetForwardedPortOffset();
  }
  return this->ListeningPort;
};