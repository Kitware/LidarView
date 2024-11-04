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
  vtkStreamPacketHandler::Parameters params;
  if (this->IsForwarding)
  {
    params.forwardAddress = this->ForwardedIpAddress;
    params.forwardPorts.emplace_back(this->ForwardedPort);
  }
  params.listeningAddress = this->LocalListeningAddress;
  params.multicastAddress = this->MulticastAddress;
  params.listeningPorts.emplace_back(this->ListeningPort);

  this->Start(params);
}

//----------------------------------------------------------------------------
void vtkStream::Start(vtkStreamPacketHandler::Parameters& params)
{
  if (this->PacketHandler)
  {
    auto consumeCallback = [this](const std::vector<uint8_t>& pkt, double timestamp)
    { this->ConsumePacket(pkt, timestamp); };
    this->PacketHandler->StartListening(params, consumeCallback);
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
