/*=========================================================================

  Program: LidarView
  Module:  vtkUDPSenderAlgorithm.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUDPSenderAlgorithm.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <algorithm>
#include <limits>

vtkStandardNewMacro(vtkUDPSenderAlgorithm);

//-----------------------------------------------------------------------------
class vtkUDPSenderAlgorithm::vtkInternals
{
public:
  boost::asio::io_service IOService;
  boost::asio::ip::udp::socket Socket;
  boost::asio::ip::udp::endpoint Endpoint;

  vtkInternals()
    : Socket(IOService)
  {
  }
};

//-----------------------------------------------------------------------------
vtkUDPSenderAlgorithm::vtkUDPSenderAlgorithm()
  : Internals(new vtkUDPSenderAlgorithm::vtkInternals())
{
}

//-----------------------------------------------------------------------------
vtkUDPSenderAlgorithm::~vtkUDPSenderAlgorithm() = default;

//-----------------------------------------------------------------------------
int vtkUDPSenderAlgorithm::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Enabled)
  {
    return 1;
  }

  auto& internals = *(this->Internals);

  boost::system::error_code error;
  auto address = boost::asio::ip::make_address(this->IPAddress, error);
  if (error)
  {
    vtkErrorMacro("Could not determine a valid address from input string.");
    return 0;
  }

  internals.Endpoint = boost::asio::ip::udp::endpoint(address, this->DestinationPort);
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkUDPSenderAlgorithm::OpenSocket()
{
  auto& internals = *(this->Internals);
  boost::system::error_code error;
  internals.Socket.open(internals.Endpoint.protocol(), error);

  if (error)
  {
    vtkErrorMacro("Could not open socket. (error: " << error.message() << ")");
    return false;
  }
  internals.Socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  internals.Socket.set_option(boost::asio::ip::multicast::enable_loopback(true));
  return true;
}

//-----------------------------------------------------------------------------
void vtkUDPSenderAlgorithm::SendData(const uint8_t* buffer, uint16_t packetSize)
{
  auto& internals = *(this->Internals);

  if (!this->Enabled)
  {
    return;
  }
  internals.Socket.send_to(boost::asio::buffer(buffer, packetSize), internals.Endpoint);
}

//-----------------------------------------------------------------------------
void vtkUDPSenderAlgorithm::CloseSocket()
{
  auto& internals = *(this->Internals);
  internals.Socket.close();
}

//-----------------------------------------------------------------------------
uint16_t vtkUDPSenderAlgorithm::CopyData(uint8_t* buffer,
  uint16_t index,
  const void* data,
  size_t dataSize)
{
  std::memcpy(buffer + index, data, dataSize);
  return index + dataSize;
}

//-----------------------------------------------------------------------------
void vtkUDPSenderAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IPAddress: " << this->IPAddress << endl;
  os << indent << "DestinationPort: " << this->DestinationPort << endl;
}
