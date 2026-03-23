/*=========================================================================

  Program: LidarView
  Module:  vtkUDPSenderAlgorithm.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUDPSenderAlgorithm_h
#define vtkUDPSenderAlgorithm_h

#include "lvIONetworkModule.h" // For export macro
#include <vtkLiveSourceAlgorithm.h>
#include <vtkPassInputTypeAlgorithm.h>
#include <vtkSmartPointer.h> // for ivar

#include <memory>
#include <string>

#ifndef __VTK_WRAP__
#define vtkPassInputTypeAlgorithm vtkLiveSourceAlgorithm<vtkPassInputTypeAlgorithm>
#endif

/**
 * Base vtk algorithm class to send UDP data over network.
 */
class LVIONETWORK_EXPORT vtkUDPSenderAlgorithm : public vtkPassInputTypeAlgorithm
{
public:
  static vtkUDPSenderAlgorithm* New();
  vtkTypeMacro(vtkUDPSenderAlgorithm, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/disable this filter. When disabled, this filter passes data object
   * doing nothing more.
   */
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);
  ///@}

  ///@{
  /**
   * IP address to send data over. Can either be a IPv4 address string in dotted decimal form,
   * or an IPv6 address in hexadecimal notation.
   */
  vtkGetMacro(IPAddress, std::string);
  vtkSetMacro(IPAddress, std::string);
  ///@}

  ///@{
  /**
   * Port on which to send data.
   */
  vtkGetMacro(DestinationPort, int);
  vtkSetMacro(DestinationPort, int);
  ///@}

  /**
   * Copy data into a buffer at index and return the new index.
   */
  static uint16_t CopyData(uint8_t* buffer, uint16_t index, const void* data, size_t dataSize);

protected:
  vtkUDPSenderAlgorithm();
  ~vtkUDPSenderAlgorithm() override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  ///@{
  /**
   * Methods to open, close and send data with the UDP socket.
   */
  bool OpenSocket();
  void SendData(const uint8_t* buffer, uint16_t packetSize);
  void CloseSocket();
  ///@}

private:
  vtkUDPSenderAlgorithm(const vtkUDPSenderAlgorithm&) = delete;
  void operator=(const vtkUDPSenderAlgorithm&) = delete;

  bool Enabled = true;
  int DestinationPort = 8888;
  std::string IPAddress;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#ifndef __VTK_WRAP__
#undef vtkPassInputTypeAlgorithm
#endif

#endif
